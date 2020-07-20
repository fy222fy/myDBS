#include "VFS.h"
VFS::VFS():is_active(false),nums(0){}

VFS* VFS::get_VFS(Options *op){//获得单例模式的VFS
    static VFS vfs;
    if(!vfs.is_active){//如果是首次打开，要先从文件中读取信息，或者创建文件
        if(op == nullptr){
            assert("首次调用vfs必须指定op");
        }
        if(!DataFile::if_exist(vfs.fname,op)){
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
        M[tmp_id] = p_addr; //添加到map中
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

Status VFS::read_seg_head(BlockHandle *bh, SegHead **sh){
    Status s;
    vector<uint8_t> result;
    df->read_block(bh,result);//先读出来
    if(result.size() == 0) s.FetalError("头部分未赋值");
    *sh = new SegHead();
    (*sh)->ID = int8_to_int32(result,0);//反序列ID
    (*sh)->meta_first_addr = int8_to_int32(result,4);//反序列首元数据地址
    return s;
}

Status VFS::write_seg_head(BlockHandle *bh, SegHead *sh){
    Status s;
    vector<uint8_t> result(BlockHead::FREE_SPACE,0x00);
    uint8_t *temp = int32_to_int8(sh->ID);//首先写入ID
    for(int i = 0; i < 4; i++) result[i] = temp[i];
    temp = int32_to_int8(sh->meta_first_addr);
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
    for(int i = 0; i < VFS::META_NUMS; i++,j+=8){
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

Status VFS::create_seg(uint32_t id){
    Status s;
    if(!is_active) s.FetalError("VFS还未打开文件，请查看文件是否已经被打开");
    if(M.count(id) != 0) s.FetalError("已经创建过该段，无法继续创建");
    BlockHandle *bh;
    s = df->alloc_block(&bh);//创建一个块，做为段头块
    M[id] = bh->address;
    BlockHandle *bh2;
    s = df->alloc_block(&bh2);//为这个段生成第一个元数据信息块
    SegHead sh(id,bh2->address);
    write_seg_head(bh,&sh);
    SegMeta sm;
    write_seg_meta(bh2,&sm);//写入一个空的元数据文件块
    return s;
}

Status VFS::free_seg(uint32_t id){
    Status s;
    if(!is_active) s.FetalError("VFS还未打开文件，请查看文件是否已经被打开");
    bool state = true;
    int i =0;
    if(M.count(id) == 0) s.FetalError("系统中不存在你要删除的段，确定id正确？");
    uint32_t addr = M[id];
    BlockHandle bh_head(addr);//段头的位置
    SegHead *sh;
    read_seg_head(&bh_head,&sh);//读取段头结构
    BlockHandle bh2(sh->meta_first_addr);//获得第一个元数据信息块地址
    free_all_page(&bh2);//递归释放所有的块，不用再释放bh一次了
    df -> free_block(&bh_head);//段头页也释放
    M.erase(id);//从M中删除段头的信息
    nums--;
    //下面写入新的段info
    write_add_seg_info();
    delete sh;
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
        df->free_block(bh);//把元数据信息也free
        delete sm;
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
        df->free_block(bh);//把最顶层的bh也释放掉
        delete sm;
        return s;
    }
    return s;
}


Status VFS::append(uint32_t id, uint32_t &offset, const vector<uint8_t> &data,uint32_t beg,uint32_t len){
    Status s;
    if(M.count(id) == 0) s.FetalError("系统中不存在你要删除的段，确定id正确？");
    BlockHandle head_addr(M[id]);
    SegHead *sh;
    read_seg_head(&head_addr,&sh);
    BlockHandle *bh = new BlockHandle(sh->meta_first_addr);
    BlockHandle *p_bh;//最后对应的物理地址
    SegMeta *sm;
    read_seg_meta(bh,&sm);//读出首个元数据信息
    //首先遍历已分配的页，如果有就分配出去
    //如果没有，就找到最后，新建一个页
    bool state = false;
    while(true){
        for(int i = 0; i < META_NUMS; i++){
            if(sm->M[i].first == 0x00){//找到了空的页
                offset = i;
                df->alloc_block(&p_bh);
                sm->M[i].second = p_bh->address;
                sm->M[i].first = 0x01;
                write_seg_meta(bh,sm);
                state = true;//外层也退出
                break;
            }
        }
        if(state) break;//找到了退出
        if(sm->if_have_next != 0x00){//还有下一个元数据块
            bh = new BlockHandle(sm->next_meta_addr);
            read_seg_meta(bh,&sm);//读出下一个元数据块继续循环
        }
        else break;//否则就说明所有已分配的页都查看过了，并且没有
    }
    if(!state){//如果还是没有找到，则可以申请新的元数据信息块了
        sm->if_have_next = 0x01;
        BlockHandle *bh2;
        df -> alloc_block(&bh2);
        sm->next_meta_addr = bh2->address;
        write_seg_meta(bh,sm);
        SegMeta *sm2;
        df -> alloc_block(&p_bh);//申请一个新的块来写入
        sm2->M[0] = make_pair(0x01,p_bh->address);
        write_seg_meta(bh2,sm2);//把心申请的元数据块写入文件
        delete sm2;
        delete bh2;
    }
    //到此，p_bh是最终的块，现在写入数据
    df -> write_block(data,beg,len,p_bh);
    delete bh;
    delete p_bh;
    delete sm;
    delete sh;
    return s;
}

Status VFS::write_page(uint32_t id, uint32_t offset, const vector<uint8_t> &data,uint32_t beg,uint32_t len){
    Status s;
    if(M.count(id) == 0) s.FetalError("系统中不存在你要删除的段，确定id正确？");
    BlockHandle head_addr(M[id]);
    SegHead *sh;
    read_seg_head(&head_addr,&sh);
    int num = offset / META_NUMS;//要跳过几个元数据信息
    uint32_t own_offset = offset % META_NUMS;//最后一个元数据的内部偏移
    BlockHandle *bh = new BlockHandle(sh->meta_first_addr);
    SegMeta *sm;
    read_seg_meta(bh,&sm);//读出首个元数据信息
    for(int i = 0; i < num; i++){
        if (sm->if_have_next == 0x00){
            //如果已经是最后一个块了，那么说明需要分配新的块来满足偏移
            BlockHandle *bh2;
            df->alloc_block(&bh2);//创建一个新的元数据信息块
            sm->if_have_next = 0x01;
            sm->next_meta_addr = bh2->address;
            write_seg_meta(bh,sm);
            //之后再写新的元数据块
            SegMeta *sm2 = new SegMeta();
            write_seg_meta(bh2,sm2);//写入新的块，还是不一定够
            delete bh;
            bh = bh2;//bh切换到新的元数据块上来
            sm = sm2;
        }
        else{//如果不是最后一个块，则继续后移
            bh = new BlockHandle(sm->next_meta_addr);
            read_seg_meta(bh,&sm);
        }
        
    }
    //现在已经定位到了最后一个块 sm bh
    if(sm->M[own_offset].first == 0x01){ //表明该页已经在使用
        s.FetalError("该页正在使用，继续写入会覆盖原有的值");
    }
    //现在为该页分配新的块，然后写入
    BlockHandle *bbtemp;
    df->alloc_block(&bbtemp);
    sm->M[own_offset] = make_pair(0x01,bbtemp->address);
    write_seg_meta(bh,sm);
    df->write_block(data,beg,len,bbtemp);
    delete sh;
    delete sm;
    delete bh;
    delete bbtemp;
    return s;
}

Status VFS::read_page(uint32_t id, uint32_t offset, vector<uint8_t> &data){
    Status s;
    if(M.count(id) == 0) s.FetalError("系统中不存在你要删除的段，确定id正确？");
    BlockHandle head_addr(M[id]);
    SegHead *sh;
    read_seg_head(&head_addr,&sh);
    int num = offset / META_NUMS;//要跳过几个元数据信息
    uint32_t own_offset = offset % META_NUMS;//最后一个元数据的内部偏移
    BlockHandle *bh = new BlockHandle(sh->meta_first_addr);
    SegMeta *sm;
    read_seg_meta(bh,&sm);//读出首个元数据信息
    for(int i = 0; i < num; i++){
        if (sm->if_have_next == 0x00){//说明这已经是最后一个块了
            s.FetalError("尝试读取还没有分配过的页，大概率是使用方出了问题");
        }
        bh = new BlockHandle(sm->next_meta_addr);
        read_seg_meta(bh,&sm);
    }
    if(sm->M[own_offset].first == 0x00){
        s.FetalError("该页已经free，无法读取");
    }
    BlockHandle *read_b = new BlockHandle(sm->M[own_offset].second);
    s = df -> read_block(read_b, data);
    delete bh;
    delete sh;
    delete sm;
    delete read_b;
    return s;
}

Status VFS::free_page(uint32_t id, uint32_t offset){
    Status s;
    if(M.count(id) == 0) s.FetalError("系统中不存在你要删除的段，确定id正确？");
    BlockHandle head_addr(M[id]);
    SegHead *sh;
    read_seg_head(&head_addr,&sh);
    int num = offset / META_NUMS;//要跳过几个元数据信息
    uint32_t own_offset = offset % META_NUMS;//最后一个元数据的内部偏移
    BlockHandle *bh = new BlockHandle(sh->meta_first_addr);
    SegMeta *sm;
    read_seg_meta(bh,&sm);//读出首个元数据信息
    for(int i = 0; i < num; i++){
        if (sm->if_have_next == 0x00){//说明这已经是最后一个块了
            s.FetalError("尝试free还没有分配过的页，大概率是使用方出了问题");
        }
        bh = new BlockHandle(sm->next_meta_addr);
        read_seg_meta(bh,&sm);
    }
    if(sm->M[own_offset].first == 0x00){
        s.FetalError("该页已经free，不能再次free");
    }
    BlockHandle *tempb = new BlockHandle(sm->M[own_offset].second);
    df->free_block(tempb);
    sm->M[own_offset] = make_pair(0x00,0x00);//置零
    write_seg_meta(bh,sm);
    delete bh;
    delete tempb;
    return s;
}