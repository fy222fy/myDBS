#include "VFS.h"

Segment::Segment(uint32_t id, DataFile* ddf):ID(id),df(ddf){
}
Segment::~Segment(){
    
}

Status Segment::append(uint32_t &offset, const vector<uint8_t> &data,uint32_t beg,uint32_t len){
    Status s;
    BlockHandle *bh = new BlockHandle(meta_first_addr);
    BlockHandle *p_bh;//最后对应的物理地址
    SegMeta *sm;
    VFS::get_VFS() -> read_seg_meta(bh,&sm);//读出首个元数据信息
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
                VFS::get_VFS() -> write_seg_meta(bh,sm);
                state = true;//外层也退出
                break;
            }
        }
        if(state) break;//找到了退出
        if(sm->if_have_next != 0x00){//还有下一个元数据块
            bh = new BlockHandle(sm->next_meta_addr);
            VFS::get_VFS() -> read_seg_meta(bh,&sm);//读出下一个元数据块继续循环
        }
        else break;//否则就说明所有已分配的页都查看过了，并且没有
    }
    if(!state){//如果还是没有找到，则可以申请新的元数据信息块了
        sm->if_have_next = 0x01;
        BlockHandle *bh2;
        df -> alloc_block(&bh2);
        sm->next_meta_addr = bh2->address;
        VFS::get_VFS() -> write_seg_meta(bh,sm);
        SegMeta *sm2 = new SegMeta();
        df -> alloc_block(&p_bh);//申请一个新的块来写入
        pair<uint32_t,uint32_t> mp = make_pair(0x01,p_bh->address);
        sm2->M[0] = mp;
        VFS::get_VFS() -> write_seg_meta(bh2,sm2);//把心申请的元数据块写入文件
        delete bh2;
        delete sm2;
    }
    //到此，p_bh是最终的块，现在写入数据
    df -> write_block(data,beg,len,p_bh);
    delete bh;
    delete p_bh;
    delete sm;
    return s;
}

Status Segment::write_page(uint32_t offset, const vector<uint8_t> &data,uint32_t beg,uint32_t len){
    Status s;
    int num = offset / META_NUMS;//要跳过几个元数据信息
    uint32_t own_offset = offset % META_NUMS;//最后一个元数据的内部偏移
    BlockHandle *bh = new BlockHandle(meta_first_addr);
    SegMeta *sm;
    VFS::get_VFS() -> read_seg_meta(bh,&sm);//读出首个元数据信息
    for(int i = 0; i < num; i++){
        if (sm->if_have_next == 0x00){
            //如果已经是最后一个块了，那么说明需要分配新的块来满足偏移
            BlockHandle *bh2;
            df->alloc_block(&bh2);//创建一个新的元数据信息块
            sm->if_have_next = 0x01;
            sm->next_meta_addr = bh2->address;
            VFS::get_VFS() -> write_seg_meta(bh,sm);
            //之后再写新的元数据块
            SegMeta *sm2 = new SegMeta();
            VFS::get_VFS() -> write_seg_meta(bh2,sm2);//写入新的块，还是不一定够
            delete bh;
            delete sm;
            bh = bh2;//bh切换到新的元数据块上来
            sm = sm2;
        }
        else{//如果不是最后一个块，则继续后移
            bh = new BlockHandle(sm->next_meta_addr);
            VFS::get_VFS() -> read_seg_meta(bh,&sm);
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
    VFS::get_VFS() -> write_seg_meta(bh,sm);
    df->write_block(data,beg,len,bbtemp);
    delete bh;
    delete sm;
    delete bbtemp;
    return s;
}

Status Segment::read_page(uint32_t offset, vector<uint8_t> &data){
    Status s;
    int num = offset / META_NUMS;//要跳过几个元数据信息
    uint32_t own_offset = offset % META_NUMS;//最后一个元数据的内部偏移
    BlockHandle *bh = new BlockHandle(meta_first_addr);
    SegMeta *sm;
    VFS::get_VFS() -> read_seg_meta(bh,&sm);//读出首个元数据信息
    for(int i = 0; i < num; i++){
        if (sm->if_have_next == 0x00){//说明这已经是最后一个块了
            s.FetalError("尝试读取还没有分配过的页，大概率是使用方出了问题");
        }
        bh = new BlockHandle(sm->next_meta_addr);
        VFS::get_VFS() -> read_seg_meta(bh,&sm);
    }
    if(sm->M[own_offset].first == 0x00){
        s.FetalError("该页已经free，无法读取");
    }
    BlockHandle *read_b = new BlockHandle(sm->M[own_offset].second);
    s = df -> read_block(read_b, data);
    delete bh;
    delete sm;
    delete read_b;
    return s;
}

Status Segment::free_page(uint32_t offset){
    Status s;
    int num = offset / META_NUMS;//要跳过几个元数据信息
    uint32_t own_offset = offset % META_NUMS;//最后一个元数据的内部偏移
    BlockHandle *bh = new BlockHandle(meta_first_addr);
    SegMeta *sm;
    VFS::get_VFS() -> read_seg_meta(bh,&sm);//读出首个元数据信息
    for(int i = 0; i < num; i++){
        if (sm->if_have_next == 0x00){//说明这已经是最后一个块了
            s.FetalError("尝试free还没有分配过的页，大概率是使用方出了问题");
        }
        bh = new BlockHandle(sm->next_meta_addr);
        VFS::get_VFS() -> read_seg_meta(bh,&sm);
    }
    if(sm->M[own_offset].first == 0x00){
        s.FetalError("该页已经free，不能再次free");
    }
    BlockHandle *tempb = new BlockHandle(sm->M[own_offset].second);
    df->free_block(tempb);
    sm->M[own_offset] = make_pair(0x00,0x00);//置零
    VFS::get_VFS() -> write_seg_meta(bh,sm);
    delete bh;
    delete sm;
    delete tempb;
    return s;
}





