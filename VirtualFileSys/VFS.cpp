#include "VFS.h"
VFS::VFS():is_active(false),nums(0){}

VFS* VFS::get_VFS(Options *op){//获得单例模式的VFS
    static VFS vfs;
    if(!vfs.is_active){//如果是首次打开，要先从文件中读取信息，或者创建文件
        if(op == nullptr){
            assert("首次调用vfs必须指定op");
        }
        if(!DataFile::if_exist(vfs.fname)){
            DataFile::create_datafile(vfs.fname,op,&vfs.df);//创建数据文件
            vfs.is_active = true;
            vfs.write_add_seg_info();//将头部信息写入数据文件头部
        }
        else{
            DataFile::open_datafile(vfs.fname,op,&vfs.df);//打开数据文件
            vfs.read_seg_info();//从头部文件中读入所有的段信息
        }
    }
    return &vfs;
}

VFS *VFS::get_VFS(){
    return get_VFS(nullptr);
}

Status VFS::read_seg_info(){
    Status s;
    BlockHandle *bh;
    df->get_first_bh(&bh);//获取第一个bh
    vector<uint8_t> result;
    df -> read_block(bh,result);//先读出来
    if(result.size() != 0){//如果没读出东西来
        nums = int8_to_int32(result,0);
    }
    int j = 8;
    for(int i = 0; i < nums; i++,j+=8){
        uint32_t tmp_id = int8_to_int32(result,j);
        uint32_t p_addr = int8_to_int32(result, j+4);
        M.emplace_back(make_pair(tmp_id,p_addr)); //添加到map中
    }
    delete bh;
    return s;
}

Status VFS::write_add_seg_info(){
    Status s;
    BlockHandle *bh;
    df->get_first_bh(&bh);//获取第一个bh
    vector<uint8_t> input(BlockHead::FREE_SPACE,0x00);
    uint8_t *temp = int32_to_int8(M.size());
    for(int i = 0; i< 4;i++) input[i] = temp[i];
    int findex =8;
    for(auto item:(M)){
        temp = int32_to_int8(item.first);
        for(int i = 0; i< 4;i++) input[findex+i] = temp[i];
        findex += 4;
        temp = int32_to_int8(item.second);
        for(int i = 0; i< 4;i++) input[findex+i] = temp[i];
        findex += 4;
    }
    df -> write_block(input,0,input.size(),bh);
    delete temp;
    delete bh;
    return s;
}

Status VFS::create_seg(uint32_t id, Segment **sg){
    Status s;
    if(!is_active) s.FetalError("VFS还未打开文件，请查看文件是否已经被打开");
    for(auto p : M){
        if(p.first == id){
            s.FetalError("已经创建过该段，请直接调用open打开");
            return s;
        }          
    }
    *sg = new Segment(id, df);//创建一个段
    BlockHandle *bh;
    s = df->alloc_block(&bh);//创建一个块，做为段头
    M.emplace_back(make_pair(id,bh->address));
    BlockHandle *bh2;
    s = df->alloc_block(&bh2);//为这个段生成第一个元数据信息块
    (*sg) -> set_meta_addr(bh2);//把第一个元数据信息快写入段头
    s = (*sg) -> write_seg_head(bh); //把段头内容写到这第一个块里
    SegMeta *sm = new SegMeta();
    write_seg_meta(bh2,sm);//写入一个空的元数据文件块
    delete sm;
    return s;
}

Status VFS::open_seg(uint32_t id, Segment **sg){
    Status s;
    if(!is_active) s.FetalError("VFS还未打开文件，请查看文件是否已经被打开");
    bool state = true;
    uint32_t addr;
    for(auto p : M){
        if(p.first == id){
            addr = p.second;
            state = false;
            break;
        }
    }
    if(state) s.FetalError("还没有创建段就要打开，请先使用create创建该段");
    *sg = new Segment(id,df); //初始化一个段，后面还要从文件头中读取段的信息
    BlockHandle *bh = new BlockHandle(addr);//段头的位置
    (*sg) -> read_seg_head(bh);//读取段头结构
    BlockHandle *bh2;
    (*sg) -> get_meta_addr(&bh2);//得到段的第一个元数据信息的地址的bh
    return s;
}

Status VFS::free_seg(uint32_t id){
    Status s;
    if(!is_active) s.FetalError("VFS还未打开文件，请查看文件是否已经被打开");
    bool state = true;
    int i =0;
    uint32_t addr;
    for(; i < nums;i++){
        if(M[i].first == id){
            addr = M[i].second;
            state = false;
            break;
        }
    }
    if(state) s.FetalError("系统中不存在你要删除的段，确定id正确？");
    Segment seg(id,df); //初始化一个段，后面还要从文件头中读取段的信息
    BlockHandle *bh_head = new BlockHandle(addr);//段头的位置
    seg.read_seg_head(bh_head);//读取段头结构
    BlockHandle *bh2;
    seg.get_meta_addr(&bh2);//得到段的第一个元数据信息的地址的bh2
    free_all_page(bh2);//递归释放所有的块，不用再释放bh一次了
    df -> free_block(bh_head);//段头页也释放
    M.erase(M.begin() + i);//从M中删除段头的信息
    nums--;
    //下面写入新的段info
    write_add_seg_info();
    return s;
}

Status VFS::free_all_page(BlockHandle *bh){
    Status s;
    SegMeta *sm;
    read_seg_meta(bh,&sm);//从bh中读出元数据信息
    if(sm->if_have_next == 0x00){//如果没有就释放并终止
        for(auto item : sm->M){
            if(item.first != 0x00){//如果存在已经分配的块
                BlockHandle *tmpb = new BlockHandle(item.second);
                s = df->free_block(tmpb);
                item.first = 0x00;
            }
        }
        delete sm;
        df->free_block(bh);//把元数据信息也free
        return s;
    }
    else{
        BlockHandle *bh_next = new BlockHandle(sm->next_meta_addr);//下一个元数据块
        free_all_page(bh_next);//下面先释放
        for(auto item : sm->M){
            if(item.first != 0x00){//如果存在已经分配的块
                BlockHandle *tmpb = new BlockHandle(item.second);
                s = df->free_block(tmpb);
                item.first = 0x00;
            }
        }
        delete sm;
        df->free_block(bh);//把最顶层的bh也释放掉
        return s;
    }
    return s;
}

Status VFS::read_seg_head(BlockHandle *bh){
    Status s;
    vector<uint8_t> result;
    df->read_block(bh,result);//先读出来
    if(result.size() == 0) s.FetalError("头部分未赋值");
    ID = int8_to_int32(result,0);//反序列ID
    meta_first_addr = int8_to_int32(result,4);//反序列首元数据地址
    return s;
}

Status VFS::write_seg_head(BlockHandle *bh){
    Status s;
    vector<uint8_t> result(BlockHead::FREE_SPACE,0x00);
    uint8_t *temp = int32_to_int8(ID);//首先写入ID
    for(int i = 0; i < 4; i++) result[i] = temp[i];
    temp = int32_to_int8(meta_first_addr);
    for(int i = 0; i < 4; i++) result[4 + i] = temp[i];
    df->write_block(result,0,result.size(),bh);
    delete temp;
    return s;
}

Status VFS::read_seg_meta(BlockHandle *bh, SegMeta **sm){
    Status s;
    *sm = new SegMeta();
    vector<uint8_t> result;
    df->read_block(bh,result);//读取出这个块的所有的meta
    if(result.size() == 0){
        s.FetalError("没有成功从块中读取出任何元数据信息");
    }
    (*sm) ->if_have_next = int8_to_int32(result,0);
    (*sm)->next_meta_addr = int8_to_int32(result,4);
    int j = 8;
    for(int i = 0; i < Segment::META_NUMS; i++,j+=8){
        uint32_t state = int8_to_int32(result,j);
        uint32_t p_addr = int8_to_int32(result, j+4);
        (*sm) -> M[i] = make_pair(state,p_addr); //添加到map中
    }
    return s;
}

Status VFS::write_seg_meta(BlockHandle *bh, SegMeta *sm){
    Status s;
    vector<uint8_t> result(BlockHead::FREE_SPACE,0x00);
    uint8_t *temp = int32_to_int8(sm->if_have_next);
    for(int i = 0;i<4;i++) result[i] = temp[i];
    temp = int32_to_int8(sm->next_meta_addr);
    for(int i = 0;i<4;i++) result[4+i] = temp[i];
    int findex = 8;
    for(auto item:sm->M){
        uint32_t state = item.first;
        temp = int32_to_int8(state);
        for(int i = 0;i<4;i++) result[findex+i] = temp[i];
        findex+=4;
        temp = int32_to_int8(item.second);
        for(int i = 0;i<4;i++) result[findex+i] = temp[i];
        findex+=4;
    }
    df->write_block(result,0,result.size(),bh);
    return s;
}