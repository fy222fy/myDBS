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
LOBimpl::LOBimpl(LOBLocator *ll):ll(ll),segid(ll->get_seg_id()),cur_version(ll->get_version()+1),mode(ll->get_mode()),if_inrow(ll->get_mode()==0x10){
    if(ll->get_mode()==0x10){
        set_inrow_data(ll->get_data(),ll->get_data_size());
    }
    else if(ll->get_mode()==0x11){
        for(auto item:ll->get_all_lpas()){
           all_lpa.emplace_back(make_pair(item.first,item.second));
        }
    }
    else if(ll->get_mode() == 0x12){
        LHP lhp;
        read_LHP(ll->get_LHPA(),lhp);
        for(auto item:lhp.get_all_lpas()) all_lpa.emplace_back(item);
    }
    else{
        LHIP lhip;
        read_LHIP(ll->get_LHIPA(),lhip);
        for(auto lhps:lhip.get_all_lhps()){
            LHP lhp;
            read_LHP(lhps.first,lhp);
            for(auto item:lhp.get_all_lpas()) all_lpa.emplace_back(item);
        }
    }
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
Status LOBimpl::append(const uint8_t *data, uint64_t amount){
    Status s;
    if(amount + ll->get_data_size() > LOBimpl::OUTLINE_3_MAX_SIZE) s.FetalError("append的数据超过最大限制");
    uint64_t new_len = data_size + amount;
    uint8_t temp[new_len];
    memcpy(temp,inrow_data,data_size);
    memcpy(temp+data_size,data,amount);
    if(new_len <= LOBLocator::INLINE_MAX_SIZE){//表示可以行内存
        set_inrow_data(temp,new_len);
    }
    else{//需要写入文件了
        int beg = 0;
        for(int beg = 0; beg + LOB_PAGE_SIZE < new_len; beg+=LOB_PAGE_SIZE){
            uint64_t offset;
            create_lobpage(offset,temp+beg,LOB_PAGE_SIZE);
            add_lpa(offset,LOB_PAGE_SIZE);
        }
        if(beg < new_len){
            uint64_t offset;
            create_lobpage(offset,temp+beg,new_len - beg);
            add_lpa(offset,LOB_PAGE_SIZE);
        }
        free_inrow_data();
    }
    return s;
}

Status LOBimpl::insert(uint64_t data_off, const uint8_t *data, uint64_t amount){
    Status s;
    if(amount + ll->get_data_size() > LOBimpl::OUTLINE_3_MAX_SIZE) s.FetalError("insert的数据超过最大限制");
    if(data_off == data_size) append(data,amount);
    if(if_inrow){//如果当前是行内存//直接转换成普通的append
        uint8_t temp[amount+data_size];
        memcpy(temp,inrow_data,data_off);
        memcpy(temp,data,amount);
        memcpy(temp,inrow_data+data_off+amount,data_size-data_off);
        append(temp,amount+data_size);
    }
    uint64_t cur_off = 0;
    for(uint64_t i = 0; i < all_lpa.size();i++){
        if(cur_off + all_lpa[i].second > data_off){//找到了位置！
            uint8_t temp[all_lpa[i].second];
            uint64_t temp_off = data_off - cur_off;
            uint64_t offset;
            read_lobpage(offset,temp,all_lpa[i].second);
            create_lobpage(offset,temp,temp_off);//前半段
            all_lpa[i].first = offset;
            all_lpa[i].second = temp_off;
            create_lobpage(offset,temp+temp_off,all_lpa[i].second-temp_off);
            all_lpa.insert(all_lpa.begin()+i+1,make_pair(offset,all_lpa[i].second-temp_off));
            i++;
            int beg = 0;
            for(int beg = 0; beg + LOB_PAGE_SIZE < amount; beg+=LOB_PAGE_SIZE){
                uint64_t offset;
                create_lobpage(offset,temp+beg,LOB_PAGE_SIZE);
                all_lpa.insert(all_lpa.begin()+i,make_pair(offset,LOB_PAGE_SIZE));
                i++;
            }
            if(beg < amount){
                uint64_t offset;
                create_lobpage(offset,temp+beg,amount - beg);
                all_lpa.insert(all_lpa.begin()+i,make_pair(offset,amount - beg));
                i++;
            }
            free_inrow_data();
            break;
        }
        else{
            cur_off += all_lpa[i].second;
        }
    }
    return s;
}
Status LOBimpl::read(uint64_t data_off, uint8_t *result, uint64_t amount){
    Status s;
    if(amount + data_off > data_size) s.FetalError("读取位置超出最大长度");
    if(if_inrow){//如果当前是行内存//直接读取指定
        memcpy(result,inrow_data+data_off,amount);
    }
    uint64_t cur_off = 0;
    uint64_t result_off = 0;//表示结果读到多少了
    int state = 0;//0表示正在找，1表示正在连续读块，2表示已经完成
    for(uint64_t i = 0; i < all_lpa.size();i++){
        if(state == 0){
            if(cur_off + all_lpa[i].second > data_off){//找到了位置！
                uint8_t temp[all_lpa[i].second];
                uint64_t temp_off = data_off - cur_off;
                read_lobpage(all_lpa[i].first,temp,all_lpa[i].second);
                if(all_lpa[i].second-temp_off > amount){//本块内足够读完
                    memcpy(result,temp+temp_off,amount);
                    break;
                }
                else{
                    memcpy(result,temp+temp_off,all_lpa[i].second-temp_off);//后半部分写进来
                    result_off += all_lpa[i].second-temp_off;
                    cur_off += all_lpa[i].second;
                    state = 1;
                }
            }
            else{
                cur_off += all_lpa[i].second;
            }
        }
        else if(state == 1){
            if(cur_off + all_lpa[i].second > data_off + amount){//最后一个块
                uint8_t temp[all_lpa[i].second];
                uint64_t temp_off = data_off + amount - cur_off;
                read_lobpage(all_lpa[i].first,temp,all_lpa[i].second);
                memcpy(result+result_off,temp,temp_off);
                break;
            }
            else{
                uint8_t temp[all_lpa[i].second];
                read_lobpage(all_lpa[i].first,temp,all_lpa[i].second);
                memcpy(result+result_off, temp, all_lpa[i].second);
                result_off += all_lpa[i].second;
            }
        }
    }
    return s;
}
Status LOBimpl::erase(uint64_t data_off, uint64_t amount){
    Status s;
    if(data_off > ll->get_data_size()) s.FetalError("要删除的位置比原长度还要大，请核实");
    
    return s;
}
Status LOBimpl::finish(){
    if(all_lpa.size() == 0){
        ll->set_data(data_t,data_size);
        ll->update_head(0x10,data_size);
    }
    else if(all_lpa.size()<=LOBLocator::MAX_LPA){
        for(auto item:all_lpa){
            ll->append_lpas(item.first,item.second);
        }
        ll->update_head(0x11,data_size);
    }
    else if(all_lpa.size() <= LHP::MAX_LPA){
        LHP lhp;
        for(auto item:all_lpa){
            lhp.append(item.first,item.second);
        }
        uint64_t lhpa;
        append_LHP(lhpa,lhp);
        ll->set_LHPA(lhpa);
        ll->update_head(0x12,data_size);
    }
    else{
        LHIP lhip;
        LHP lhp;
        for(auto item:all_lpa){
            if(lhp.is_full()){
                uint64_t lhpa;
                append_LHP(lhpa,lhp);
                lhip.append(lhpa,lhp.data_size);
                lhp = LHP();
            }
            lhp.append(item.first,item.second);
        }
        uint64_t lhpa;
        append_LHP(lhpa,lhp);
        lhip.append(lhpa,lhp.data_size);
        uint64_t lhipa;
        append_LHIP(lhipa,lhip);
        ll->set_LHIPA(lhipa);
        ll->update_head(0x13,data_size);
    }
    
}

///////////////////////private function///////////////////////

uint32_t LOBimpl::checksum(){
    return 0x01;
}

Status LOBimpl::create_lobpage(uint64_t &offset, const uint8_t *data, uint64_t len){
    Status s;
    VFS *vfs = VFS::get_VFS();
    LOBP lobp(data,len);
    uint8_t result[LOBP::HEAD_SIZE + len];
    lobp.Serialize(result);
    s = vfs->append(segid,offset,result,LOBP::HEAD_SIZE + len);
    return s;
}

Status LOBimpl::write_lobpage(uint64_t offset, const uint8_t *data, uint64_t len){
    Status s;
    VFS *vfs = VFS::get_VFS();
    LOBP lobp(data,len);
    uint8_t result[LOBP::HEAD_SIZE + len];
    lobp.Serialize(result);
    s = vfs -> write_page(segid,offset,result,LOBP::HEAD_SIZE + len);
    return s;
}
Status LOBimpl::read_lobpage(uint64_t offset, uint8_t *output, uint64_t len){
    Status s;
    VFS *vfs = VFS::get_VFS();
    uint8_t temp[LOBP::HEAD_SIZE + len];
    s = vfs->read_page(segid,offset,temp);
    LOBP lobp;
    lobp.Deserialize(temp);
    memcpy(output,lobp.data,len);
    return s;
}

Status LOBimpl::append_LHP(uint64_t &offset, LHP lhp){
    Status s;
    VFS *vfs = VFS::get_VFS();
    uint8_t result[LHP::LHP_SIZE] = {0x00};
    lhp.Serialize(result);
    s = vfs->append(segid,offset,result,LHP::LHP_SIZE);
    return s;
}
Status LOBimpl::write_LHP(uint64_t offset, LHP lhp){
    Status s;
    VFS *vfs = VFS::get_VFS();
    uint8_t result[LHP::LHP_SIZE] = {0x00};
    lhp.Serialize(result);
    s = vfs->write_page(segid,offset,result,LHP::LHP_SIZE);
    return s;
}
Status LOBimpl::read_LHP(uint64_t offset, LHP &lhp){
    Status s;
    VFS *vfs = VFS::get_VFS();
    uint8_t data[LHP::LHP_SIZE] = {0x00};
    vfs->read_page(segid,offset,data);
    lhp.Deserialize(data);
    return s;
}


Status LOBimpl::append_LHIP(uint64_t &offset, LHIP lhip){
    Status s;
    VFS *vfs = VFS::get_VFS();
    uint8_t result[LHP::LHP_SIZE] = {0x00};
    lhip.Serialize(result);
    s = vfs->append(segid,offset,result,LHIP::LHIP_SIZE);
    return s;
}

Status LOBimpl::write_LHIP(uint64_t offset, LHIP lhip){
    Status s;
    VFS *vfs = VFS::get_VFS();
    uint8_t result[LHIP::LHIP_SIZE] = {0x00};
    lhip.Serialize(result);
    s = vfs->write_page(segid,offset,result,LHIP::LHIP_SIZE);
    return s;
}
Status LOBimpl::read_LHIP(uint64_t offset, LHIP &lhip){
    Status s;
    VFS *vfs = VFS::get_VFS();
    uint8_t data[LHIP::LHIP_SIZE] = {0x00};
    vfs->read_page(segid,offset,data);
    lhip.Deserialize(data);
    return s;
}

