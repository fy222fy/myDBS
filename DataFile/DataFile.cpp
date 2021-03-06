#include "DataFile.h"
#include "Block.h"
#include <assert.h>


Status DataFile::init(RWFile *f, string fname){
    Status s;
    if(f == nullptr) s.FetalError("RWFILE指针为空！");
    file = f;
    filename = fname;
    return s;
}

Status DataFile::create_datafile(string fname, Options *options, DataFile *df){
    Status s;
    //创建文件
    RWFile *f;
    s = options->env->NewFile(fname,&f);//创建新文件
    //初始化一个数据文件对象
    df->init(f,fname);
    //TODO，应该有一些初始值需要写在头部，目前都是默认值
    df -> write_head();
    df -> init_map();
    BlockHandle bh;
    df -> alloc_block(bh);//分配1个块出来，为段使用
    return s;
}

Status DataFile::open_datafile(string fname, Options *options, DataFile *df){
    Status s;
    //打开文件
    RWFile *f;
    options->env->OpenFile(fname,&f);
    //初始化一个数据文件对象
    df->init(f,fname);
    //从文件中读取数据文件头信息
    df -> read_head();
    //初始化空闲表
    df -> init_map();
    return s;
}

bool DataFile::if_exist(string fname, Options *op){
    return op->env->if_exsist(fname);
}

Status DataFile::serialize_head(uint8_t *result){
    Status s;
    Convert convert;
    uint8_t *temp = convert.int32_to_int8(ID);
    result[0] = temp[0];
    result[1] = temp[1];
    result[2] = temp[2];
    result[3] = temp[3];
    result[4] = type; 
    return s;
}

Status DataFile::deserialize_head(const uint8_t *input){
    Status s;
    Convert convert;
    ID = convert.int8_to_int32(input,0);
    type = input[4];
    return s;
}

Status DataFile::read_head(){
    Status s;
    uint8_t result[HEAD_SIZE];
    s = file->Read(0,HEAD_SIZE,result); //从文件中读取头部大小的数据
    s = deserialize_head(result); //将数据反序列化，获取头部
    return s;
}



Status DataFile::write_head(){
    Status s;
    uint8_t result[HEAD_SIZE] = {0x00};
    s = serialize_head(result);
    s = file->Write(0,result,HEAD_SIZE);
    return s;
}

Status DataFile::init_map(){
    Status s;
    free_map = new map<uint64_t,bool>;//声明位图
    for(uint32_t i = HEAD_SIZE; i < file->Size(); i += BlockHead::MAX_SIZE){
        BlockHandle bh(i);
        BlockHead bhead;
        read_block_head(bh, bhead);
        (*free_map)[i] = bhead.if_free;
    }
    return s;
}

DataFile::~DataFile(){
    free_map->clear();
    file->Close();
    delete free_map;
    delete file;
}

Status DataFile::read_block_head(BlockHandle bh, BlockHead &bhead){
    Status s;
    Convert convert;
    uint8_t result[BlockHead::HEAD_SIZE] = {0x00};
    s = file->Read(bh.address,BlockHead::HEAD_SIZE,result);
    s = bhead.Deserialize(result);
    return s;
}

Status DataFile::write_block_head(BlockHandle bh, BlockHead &bhead){
    Status s;
    Convert convert;
    //序列化一下bhead
    uint8_t data[BlockHead::HEAD_SIZE] = {0x00};
    bhead.Serialize(data);
    //然后把数据写进去
    s = file->Write(bh.address,data,BlockHead::HEAD_SIZE);
    s = file->Flush();
    return s;
}

Status DataFile::alloc_block(BlockHandle &pbh){
    Status s;
    uint32_t addr;//要找到的一个块的地址
    //检查空闲链表获得空闲地址
    bool state = true;
    for(auto item:(*free_map)){
        //找到一个已经用过的空闲地址，直接给出去，修改文件状态为正在使用，并填入数据
        if(item.second){
            addr = item.first;
            state = false;
            break;
        }
    }
    if(state){//没有在空闲链表中找到空闲地址，则顺延文件
        if(file->Size() >= DF_MAX_SIZE - BlockHead::MAX_SIZE){
            s.FetalError("文件大小不足");
            return s;
        } 
        addr = file->Size();//分配最后的地址，也就是新开辟一个空间
    }
    pbh.address = addr;//构建块handle
    //下面写入块头结构
    BlockHead bhead;
    write_block_head(pbh,bhead);
    //然后直接先用0补齐块的内容，后续可能要优化这里
    uint8_t tempdata[BlockHead::FREE_SPACE] = {0};
    file->Write(pbh.address + BlockHead::HEAD_SIZE,tempdata,BlockHead::FREE_SPACE);
    s = file->Flush();
    //最后将这个分配出去的块加在空闲链表中
    (*free_map)[addr] = false;
    return s;
}

Status DataFile::free_block(BlockHandle bh){
    //TODO:先写入log文件持久化
    //然后更改空闲表和数据块头
    Status s;
    (*free_map)[bh.address] = true;//更改空闲表
    BlockHead bhead;
    bhead.set_free();
    write_block_head(bh, bhead);
    return s;
}

Status DataFile::write_block(const uint8_t *in_data, uint64_t len, BlockHandle bh){//向表中写入一些数据
    Status s;
    if(len == 0){
        s.FetalError("输入数据为空，写入是没有意义的");
        return s;
    }
    if(len > BlockHead::FREE_SPACE){
        s.FetalError("输入块的数据太大");
        return s;
    }    
    BlockHead bhead;
    bhead.used_space = len;
    s = write_block_head(bh,bhead);
    if(!s.isok()) s.FetalError("数据块头写入文件失败");
    s = file->Write(bh.address + BlockHead::HEAD_SIZE, in_data,len);
    s = file->Flush();
    if(!s.isok()) s.FetalError("数据块写入文件失败");
    return s;
}

Status DataFile::flush(){
    //目前暂时就直接把缓冲区写入文件吧
    Status s;
    file->Sync();
    return s;
}

Status DataFile::read_block(BlockHandle bh, uint8_t *result){
    Status s;
    //首先读取头部信息，看看块到底是不是真的在用，以及数据部分的长度是多少
    BlockHead bhead;
    s = read_block_head(bh, bhead);
    if(bhead.if_free) s.FetalError("块没有分配缺在进行读操作");
    //读取指定长度的数据出来
    s = file->Read(bh.address + BlockHead::HEAD_SIZE,bhead.used_space,result);
    if(!s.isok()){
        s.FetalError("读取块失败");
    }
    return s;
}

void DataFile::Close(){
    free_map->clear();
    file->Close();
}

Status DataFile::get_first_bh(BlockHandle &bh){
    Status s;
    bh.address = DataFile::HEAD_SIZE;
    return s;
}

Status DataFile::get_next_bh(BlockHandle &bh){
    Status s;
    bh.address += BlockHead::MAX_SIZE;
    return s;
}