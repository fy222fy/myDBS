#include "LOB.h"

static uint64_t min(uint64_t a, uint64_t b){
    return a > b ? b:a;
}
LOBimpl::LOBimpl(LOBLocator *loc)
    :ll(loc),
    segid(loc->get_seg_id()),
    cur_version(loc->get_version()+1),
    mode(loc->get_mode()),
    inrow_data(nullptr),
    if_inrow(loc->get_mode()==0x10),
    data_size(loc->get_data_size()){
    
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
    else if(ll->get_mode() == 0x13){
        LHIP lhip;
        read_LHIP(ll->get_LHIPA(),lhip);
        for(auto lhps:lhip.get_all_lhps()){
            LHP lhp;
            read_LHP(lhps.first,lhp);
            for(auto item:lhp.get_all_lpas()) all_lpa.emplace_back(item);
        }
    }
}

Status LOBimpl::append(const uint8_t *data, uint64_t amount){
    Status s;
    if(amount + ll->get_data_size() > LOBimpl::OUTLINE_3_MAX_SIZE) s.FetalError("append的数据超过最大限制");
    uint64_t new_len = data_size + amount;
    uint8_t *temp = new uint8_t[new_len];
    memcpy(temp,inrow_data,data_size);
    memcpy(temp+data_size,data,amount);
    if(new_len <= LOBLocator::INLINE_MAX_SIZE){//表示可以行内存
        set_inrow_data(temp,new_len);
    }
    else{//需要写入文件了
        free_inrow_data();
        int beg = 0;
        for(; beg + LOB_PAGE_SIZE < new_len; beg+=LOB_PAGE_SIZE){
            uint64_t offset;
            create_lobpage(offset,temp+beg,LOB_PAGE_SIZE);
            add_lpa(offset,LOB_PAGE_SIZE);
        }
        if(beg < new_len){
            uint64_t offset;
            create_lobpage(offset,temp+beg,new_len - beg);
            add_lpa(offset,new_len - beg);
        }
    }
    delete temp;
    return s;
}

Status LOBimpl::insert(uint64_t data_off, const uint8_t *data, uint64_t amount){
    Status s;
    if(amount + ll->get_data_size() > LOBimpl::OUTLINE_3_MAX_SIZE) s.FetalError("insert的数据超过最大限制");
    if(data_off == data_size) append(data,amount);
    if(if_inrow){//如果当前是行内存//直接转换成普通的append
        uint64_t new_len = amount+data_size;
        uint8_t *temp = new uint8_t[new_len];
        memcpy(temp,inrow_data,data_off);
        memcpy(temp+data_off,data,amount);
        memcpy(temp+data_off+amount,inrow_data+data_off,data_size-data_off);
        free_inrow_data();
        append(temp,new_len);
        delete temp;
    }
    else{
        uint64_t cur_off = 0;
        for(uint64_t i = 0; i < all_lpa.size();i++){
            if(cur_off + all_lpa[i].second > data_off){//找到了位置！
                uint8_t temp[all_lpa[i].second];
                uint64_t temp_off = data_off - cur_off; //前半段长度
                uint64_t last_off = all_lpa[i].second-temp_off ;//后半段长度 
                read_lobpage(all_lpa[i].first,temp,all_lpa[i].second);
                uint64_t offset;
                create_lobpage(offset,temp,temp_off);//前半段 
                remove_lpa(i);
                insert_lpa(offset,temp_off,i);
                create_lobpage(offset,temp+temp_off,last_off);
                insert_lpa(offset,last_off,i+1);
                i++;
                int beg = 0;
                for(; beg + LOB_PAGE_SIZE < amount; beg+=LOB_PAGE_SIZE){
                    uint64_t offset;
                    create_lobpage(offset,data+beg,LOB_PAGE_SIZE);
                    uint64_t temp_size = LOB_PAGE_SIZE;
                    insert_lpa(offset,temp_size,i);
                    i++;
                }
                if(beg < amount){
                    uint64_t offset;
                    create_lobpage(offset,data+beg,amount - beg);
                    insert_lpa(offset,amount - beg,i);
                    i++;
                }
                break;
            }
            else{
                cur_off += all_lpa[i].second;
            }
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
    else{
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
                    cur_off += all_lpa[i].second;
                    result_off += all_lpa[i].second;
                }
            }
        }
    }
    
    return s;
}
Status LOBimpl::erase(uint64_t data_off, uint64_t amount){
    Status s;
    if(data_off > ll->get_data_size()) s.FetalError("要删除的位置比原长度还要大，请核实");
    if(data_off + amount > ll->get_data_size()) amount = ll->get_data_size()-data_off;
    if(if_inrow){//如果当前是行内存//直接读取指定
        uint8_t temp[data_size - amount];
        memcpy(temp,inrow_data,data_off);
        memcpy(temp + data_off,inrow_data + data_off + amount,data_size-data_off-amount);
        set_inrow_data(temp,data_size - amount);
    }
    else{
        uint64_t cur_off = 0;
        uint64_t result_off = 0;//表示结果读到多少了
        int state = 0;//0表示正在找，1表示正在连续读块，2表示已经完成
        for(uint64_t i = 0; i < all_lpa.size();i++){
            if(state == 0){
                if(cur_off + all_lpa[i].second > data_off){//找到了位置！
                    uint8_t temp[all_lpa[i].second];
                    uint64_t temp_off = data_off - cur_off;
                    read_lobpage(all_lpa[i].first,temp,all_lpa[i].second);
                    uint64_t offset;
                    create_lobpage(offset,temp,temp_off);//前半段
                    all_lpa[i].first = offset;
                    all_lpa[i].second = temp_off;
                    i++;
                    state = 1;
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
                    uint64_t offset;
                    create_lobpage(offset,temp+data_off-cur_off,all_lpa[i].second-data_off+cur_off);
                    all_lpa[i].first = offset;
                    all_lpa[i].second = all_lpa[i].second-data_off+cur_off;
                    break;
                }
                else{
                    all_lpa.erase(all_lpa.begin()+i);
                    cur_off += all_lpa[i].second;
                    result_off += all_lpa[i].second;
                }
                
            }
        }
    }
    return s;
}
Status LOBimpl::finish(){
    Status s;
    if(if_inrow){
        ll->set_data(inrow_data,data_size);
        ll->update_head(0x10,data_size);
    }
    else{
        ll->free_data();
        if(all_lpa.size()<=LOBLocator::MAX_LPA){
            for(auto item:all_lpa){
                ll->append_lpas(item.first,item.second);
            }
            ll->update_head(0x11,data_size);
        }
        else if(all_lpa.size() <= LHP::MAX_LPA){
            LHP lhp;
            int inrow_cur_nums = 0;
            for(int i = 0; i < all_lpa.size(); i++){
                if(inrow_cur_nums < LOBLocator::MAX_LPA){
                    ll->append_lpas(all_lpa[i].first,all_lpa[i].second);
                    inrow_cur_nums++;
                }
                lhp.append(all_lpa[i].first,all_lpa[i].second);
            }
            uint64_t lhpa;
            append_LHP(lhpa,lhp);
            ll->set_LHPA(lhpa);
            ll->update_head(0x12,data_size);
        }
        else{
            LHIP lhip;
            LHP lhp;
            int inrow_cur_nums = 0;
            ll->clear_all_lpas();//清空lpas重新写
            for(auto item:all_lpa){
                if(lhp.is_full()){
                    uint64_t lhpa;
                    append_LHP(lhpa,lhp);
                    lhip.append(lhpa,lhp.data_size);
                    lhp = LHP();
                }
                if(inrow_cur_nums < LOBLocator::MAX_LPA){
                    ll->append_lpas(item.first,item.second);
                    inrow_cur_nums++;
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
    return s;
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

