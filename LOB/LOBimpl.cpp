#include "LOB.h"

static uint64_t min(uint64_t a, uint64_t b){
    return a > b ? b:a;
}

Status LOBLocator::Serialize(uint8_t *result, LOBLocator **ll){
    ;
}
Status LOBLocator::Deserialize(const uint8_t *input, LOBLocator *ll){
    ;
}

LOBimpl::LOBimpl(){
    ;
}

uint32_t LOBimpl::new_lob_id(){
    return 1001;
}

Status LOBimpl::create_locator(LOBLocator *llp, uint32_t seg_id){
    Status s;
    uint32_t lob_new_id =  new_lob_id();//创建一个LOBID
    llp->init(lob_new_id,seg_id,0x01);
    return s;
}

Status LOBimpl::create_lobseg(uint32_t &seg_id){
    Status s;
    VFS *vfs = VFS::get_VFS();//获取VFS
    seg_id = vfs->new_segid();
    vfs->create_seg(seg_id);
    return s;
}

Status LOBimpl::append(LOBLocator *ll, const uint8_t *data, uint64_t len){
    Status s;
    ll->update_version();
    uint64_t newlen = len + ll->get_data_size();
    if(newlen > LOBimpl::OUTLINE_3_MAX_SIZE){
        s.FetalError("插入的数据超过最大限制");
        return s;
    }
    uint64_t beg = 0; //这是临时全局的偏移，表示写数据的位置
    if(ll->get_mode() == 0x10){//初始行内
        uint64_t temp_size = min(LOBLocator::INLINE_MAX_SIZE,ll->get_data_size() + len);
        uint8_t *temp = new uint8_t[temp_size];
        memcpy(temp,ll->get_data(),ll->get_data_size());
        memcpy(temp+ll->get_data_size(),data,temp_size - ll->get_data_size());
        beg += temp_size - ll->get_data_size();
        ll->set_data(temp);
        ll->update_head(0x10,temp_size);//更新头部
        if(beg < len){
            //首先把data单独写进一个块，构造LPA
            uint64_t offset;
            create_lobpage(ll->get_seg_id(),offset,ll->get_data(),ll->get_data_size());
            ll->append_lpas(offset,ll->get_data_size());
            ll->update_head(0x11,ll->get_inrow_data_size());
            ll->free_data();//数据不要了
            //如果要换模式就下面操作
        }
    }
    if(ll->get_mode() == 0x11){//初始状态如果是行外1的存储，那么就继续增加
        uint64_t offset;
        for(;beg < len && !ll->is_inline_full(); beg+=LOBimpl::LOB_PAGE_SIZE){//继续填入lpa，到满足为止
            uint32_t temp_size = min(LOBimpl::LOB_PAGE_SIZE, len - beg);//找到当前最小的大小
            create_lobpage(ll->get_seg_id(),offset,data+beg,temp_size);//写入块中
            ll->append_lpas(offset,temp_size);//挨个写入lpas中
        }
        ll->update_head(0x11,ll->get_inrow_data_size());
        if(beg < len){//如果还是没存完，考虑再加LHP结构
            LHP lhp;
            for(auto item : ll->get_all_lpas()){//先把已有的行内lpa填入
                lhp.append(item.first,item.second);
            }
            uint64_t lhpa;
            append_LHP(ll->get_seg_id(),lhpa,lhp);
            ll->set_LHPA(lhpa); //保存lhpa在ll中
            ll->update_head(0x12,lhp.data_size);
        }
    }
    if(ll->get_mode() == 0x12){
        uint64_t offset;
        uint64_t lhpa = ll->get_LHPA();
        LHP lhp;
        read_LHP(ll->get_seg_id(),lhpa,lhp);//先读出lhp结构
        for(;beg < len && !lhp.is_full();beg+=LOBimpl::LOB_PAGE_SIZE){//首先先填入lhp
            uint32_t temp_size = min(LOBimpl::LOB_PAGE_SIZE, len - beg);//找到当前最小的大小
            create_lobpage(ll->get_seg_id(),offset,data+beg,temp_size);//写入块中
            lhp.append(offset,temp_size);
        }
        if(lhp.version == ll->get_version()) write_LHP(ll->get_seg_id(),lhpa,lhp);
        else append_LHP(ll->get_seg_id(),lhpa,lhp);
        ll->set_LHPA(lhpa); //不论如何，都要把这个lhp写回原来的位置
        ll->update_head(0x12,lhp.data_size);
        if(beg < len){//说明一个lhp不够存了
            LHIP lhip;
            lhip.append(ll->get_LHPA(),lhp.get_data_size());//先添加第一个情况
            uint64_t lhipa;
            append_LHIP(ll->get_seg_id(),lhipa,lhip);//写入lhip
            ll->set_LHIPA(lhipa);
            ll->update_head(0x13,lhip.data_size);
        }
    }
    if(ll->get_mode() == 0x13){
        LHIP lhip;
        LHP lhp;
        uint64_t offset;
        uint64_t lhipa = ll->get_LHIPA();
        read_LHIP(ll->get_seg_id(),lhipa,lhip);//读出lhip结构
        uint64_t lhpa = lhip.read_last_lhpa();
        read_LHP(ll->get_seg_id(),lhpa,lhp);//读出最后一个lhp结构，以追加写入
        lhip.remove_last_lhpa();//删除这最后一个lhpa
        for(;beg<len;beg+=LOBimpl::LOB_PAGE_SIZE){
            if(lhp.is_full()){
                write_LHP(ll->get_seg_id(),lhpa,lhp);
                lhip.append(lhpa,lhp.get_data_size());
                lhp = LHP();
                break;//继续新的lhp
            }
            uint32_t temp_size = min(LOBimpl::LOB_PAGE_SIZE, len - beg);//找到当前最小的大小
            create_lobpage(ll->get_seg_id(),offset,data+beg,temp_size);//写入块中
            lhp.append(offset,temp_size);
        }
        for(;beg<len;beg+=LOBimpl::LOB_PAGE_SIZE){
            if(lhp.is_full()){
                append_LHP(ll->get_seg_id(),lhpa,lhp);
                lhip.append(lhpa,lhp.get_data_size());
                lhp = LHP();
            }
            uint32_t temp_size = min(LOBimpl::LOB_PAGE_SIZE, len - beg);//找到当前最小的大小
            create_lobpage(ll->get_seg_id(),offset,data+beg,temp_size);//写入块中
            lhp.append(offset,temp_size);
        }
        append_LHP(ll->get_seg_id(),lhpa,lhp);
        lhip.append(lhpa,lhp.get_data_size());
        if(lhip.version == ll->get_version()) write_LHIP(ll->get_seg_id(),lhipa,lhip);
        else append_LHIP(ll->get_seg_id(),lhipa,lhip);
        ll->set_LHIPA(lhipa);
        ll->update_head(0x13,lhip.data_size);
    }
    //不管最终模式如何，再次更新一次locator
}
Status LOBimpl::write(LOBLocator *ll, uint64_t data_off, const uint8_t *data, uint64_t len){
    Status s;
    //首先先从行内lpa找，然后再去LHP等结构找
    
    return s;
}
Status LOBimpl::read(LOBLocator *ll, uint64_t amount, uint64_t data_off, uint8_t *result){
    Status s;
    if(ll->get_data_size() - data_off < amount) s.FetalError("要读取的长度太大，没这么长的数据");
    if(ll->get_mode() == 0x10){
        memcpy(result,ll->get_data() + data_off,amount);
        return s;
    }
    else if(ll->get_mode() == 0x11 || data_off + amount < ll->get_inrow_data_size()){//这两种情况都可以在行内找
        //首先定位到是哪一个
        uint8_t temp_result[ll->get_inrow_data_size()];
        uint64_t temp_off = 0;
        for(auto item:ll->get_all_lpas()){
            uint8_t temp[item.second];
            read_lobpage(ll->get_seg_id(),item.first,temp,item.second);//读出数据
            memcpy(temp_result + temp_off,temp,item.second);
            temp_off += item.second;
        }
        memcpy(result,temp_result+data_off,amount);
    }
    else if(ll->get_mode() == 0x12){
        uint64_t result_off = 0;
        uint64_t cur_size = 0;//用来表示当前的大小
        LHP lhp;
        read_LHP(ll->get_seg_id(),ll->get_LHPA(),lhp);//读取lhp出来
        bool state = false;
        for(auto item:lhp.get_all_lpas()){
            if(state){//说明已经应该读块了
                if(result_off + item.second > amount){//读到最后一块了
                    uint64_t temp_size = amount - result_off;
                    uint8_t temp[item.second];
                    read_lobpage(ll->get_seg_id(),item.first,temp,item.second);
                    memcpy(result+result_off,temp,temp_size);
                    break;
                }
                uint8_t temp[item.second];
                read_lobpage(ll->get_seg_id(),item.first,temp,item.second);
                memcpy(result+result_off,temp,item.second);
                result_off += item.second;
            }
            else{//说明现在还没有找到应该读的偏移，要继续往前找
                if(cur_size + item.second > data_off){//说明当前块就是了
                    //读取这个块的一部分
                    state = true;
                    uint8_t temp[item.second];
                    uint64_t temp_off = data_off - cur_size;
                    read_lobpage(ll->get_seg_id(),item.first,temp,item.second);//读这一块
                    memcpy(result + result_off,temp+temp_off,item.second-temp_off);
                    result_off += item.second - temp_off;
                }
                else{
                    cur_size += item.second;
                }
            }
        }
        
    }
    else if(ll->get_mode() == 0x13){
        uint64_t result_off = 0;
        uint64_t cur_size = 0;//用来表示当前的大小
        LHIP lhip;
        read_LHIP(ll->get_seg_id(),ll->get_LHIPA(),lhip);//读出索引
        bool state = false;//表示是否找到了应该开始读的位置
        for(auto lhps:lhip.get_all_lhps()){
            LHP lhp;
            read_LHP(ll->get_seg_id(),lhps.first,lhp);//读取出LHP
            for(auto item:lhp.get_all_lpas()){
                if(state){//说明已经应该读块了
                    if(result_off + item.second > amount){//读到最后一块了
                        uint64_t temp_size = amount - result_off;
                        uint8_t temp[item.second];
                        read_lobpage(ll->get_seg_id(),item.first,temp,item.second);
                        memcpy(result+result_off,temp,temp_size);
                        break;
                    }
                    uint8_t temp[item.second];
                    read_lobpage(ll->get_seg_id(),item.first,temp,item.second);
                    memcpy(result+result_off,temp,item.second);
                    result_off += item.second;
                }
                else{//说明现在还没有找到应该读的偏移，要继续往前找
                    if(cur_size + item.second > data_off){//说明当前块就是了
                        //读取这个块的一部分
                        state = true;
                        uint8_t temp[item.second];
                        uint64_t temp_off = data_off - cur_size;
                        read_lobpage(ll->get_seg_id(),item.first,temp,item.second);//读这一块
                        memcpy(result + result_off,temp+temp_off,item.second-temp_off);
                        result_off += item.second - temp_off;
                    }
                    else{
                        cur_size += item.second;
                    }
                }
            }
        }
    }
    return s;
}
Status LOBimpl::erase(LOBLocator *ll, uint64_t amount, uint64_t data_off){
    Status s;

    return s;
}
Status LOBimpl::drop(LOBLocator *ll){
    Status s;
    return s;
}
///////////////////////private function///////////////////////

uint32_t LOBimpl::checksum(LOBLocator *ll){
    return 0x01;
}

Status LOBimpl::create_lobpage(uint32_t segid, uint64_t &offset, const uint8_t *data, uint64_t len){
    Status s;
    VFS *vfs = VFS::get_VFS();
    LOBP lobp(data,len);
    uint8_t result[LOBP::HEAD_SIZE + len];
    lobp.Serialize(result);
    s = vfs->append(segid,offset,result,LOBP::HEAD_SIZE + len);
    return s;
}

Status LOBimpl::write_lobpage(uint32_t segid, uint64_t offset, const uint8_t *data, uint64_t len){
    Status s;
    VFS *vfs = VFS::get_VFS();
    LOBP lobp(data,len);
    uint8_t result[LOBP::HEAD_SIZE + len];
    lobp.Serialize(result);
    s = vfs -> write_page(segid,offset,result,LOBP::HEAD_SIZE + len);
    return s;
}
Status LOBimpl::read_lobpage(uint32_t segid, uint64_t offset, uint8_t *output, uint64_t len){
    Status s;
    VFS *vfs = VFS::get_VFS();
    uint8_t temp[LOBP::HEAD_SIZE + len];
    s = vfs->read_page(segid,offset,temp);
    LOBP lobp;
    lobp.Deserialize(temp);
    memcpy(output,lobp.data,len);
    return s;
}

Status LOBimpl::append_LHP(uint32_t segid, uint64_t &offset, LHP lhp){
    Status s;
    VFS *vfs = VFS::get_VFS();
    uint8_t result[LHP::LHP_SIZE] = {0x00};
    lhp.Serialize(result);
    s = vfs->append(segid,offset,result,LHP::LHP_SIZE);
    return s;
}
Status LOBimpl::write_LHP(uint32_t segid, uint64_t offset, LHP lhp){
    Status s;
    VFS *vfs = VFS::get_VFS();
    uint8_t result[LHP::LHP_SIZE] = {0x00};
    lhp.Serialize(result);
    s = vfs->write_page(segid,offset,result,LHP::LHP_SIZE);
    return s;
}
Status LOBimpl::read_LHP(uint32_t segid, uint64_t offset, LHP &lhp){
    Status s;
    VFS *vfs = VFS::get_VFS();
    uint8_t data[LHP::LHP_SIZE] = {0x00};
    vfs->read_page(segid,offset,data);
    lhp.Deserialize(data);
    return s;
}


Status LOBimpl::append_LHIP(uint32_t segid, uint64_t &offset, LHIP lhip){
    Status s;
    VFS *vfs = VFS::get_VFS();
    uint8_t result[LHP::LHP_SIZE] = {0x00};
    lhip.Serialize(result);
    s = vfs->append(segid,offset,result,LHIP::LHIP_SIZE);
    return s;
}

Status LOBimpl::write_LHIP(uint32_t segid, uint64_t offset, LHIP lhip){
    Status s;
    VFS *vfs = VFS::get_VFS();
    uint8_t result[LHIP::LHIP_SIZE] = {0x00};
    lhip.Serialize(result);
    s = vfs->write_page(segid,offset,result,LHIP::LHIP_SIZE);
    return s;
}
Status LOBimpl::read_LHIP(uint32_t segid, uint64_t offset, LHIP &lhip){
    Status s;
    VFS *vfs = VFS::get_VFS();
    uint8_t data[LHIP::LHIP_SIZE] = {0x00};
    vfs->read_page(segid,offset,data);
    lhip.Deserialize(data);
    return s;
}

