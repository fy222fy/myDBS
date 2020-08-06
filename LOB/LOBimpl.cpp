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
    if(data_off > ll->get_data_size()) s.FetalError("要写入的位置比原长度还要大，请核实");
    //首先先从行内lpa找，然后再去LHP等结构找
    uint64_t modify_len = min(data_off + len, ll->get_data_size());//这是当前修改的长度
    if(ll->get_mode() == 0x10){
        uint8_t temp[ll->get_data_size()];
        memcpy(temp,ll->get_data(),data_off);//原来的数据的前半部分
        memcpy(temp+data_off,data,modify_len);
        append(ll,data+modify_len,len-modify_len);//剩余操作用append处理。
    }
    else if(ll->get_mode() == 0x11){
        uint64_t result_off = 0;
        uint64_t cur_off = 0;
        auto lpas = ll->get_all_lpas();//获得所有的lpa
        ll->clear_all_lpas();
        int state = 0;
        for(auto item : lpas){
            if(state == 0){//还没找到位置，持续跳过
                if(cur_off + item.second > data_off){
                    state = 1;
                }
                else {
                    cur_off += item.second;
                    ll->append_lpas(item.first,item.second);
                }
            }
            if(state == 1){//找到了第一个位置，处理第一个块的后半个部分
                uint64_t temp_off = data_off - cur_off;
                uint8_t temp[LOB_PAGE_SIZE];
                read_lobpage(ll->get_seg_id(),item.first,temp,item.second);
                if(temp_off + len < item.second){//如果本块内就够读取
                    memcpy(temp+temp_off,data,len);
                    uint64_t offset;
                    create_lobpage(ll->get_seg_id(),offset,temp,LOB_PAGE_SIZE);
                    ll->append_lpas(offset,LOB_PAGE_SIZE);
                    cur_off += LOB_PAGE_SIZE;
                    state = 4;//结束
                    continue;
                }
                else {
                    memcpy(temp+temp_off,data,LOB_PAGE_SIZE-temp_off);//替换数据
                    uint64_t offset;
                    create_lobpage(ll->get_seg_id(),offset,temp,LOB_PAGE_SIZE);
                    ll->append_lpas(offset,LOB_PAGE_SIZE);
                    cur_off += LOB_PAGE_SIZE;
                    state = 2;
                    continue;
                }
            }
            if(state == 2){//处在读取状态，连续读取，并判断是不是读到最后一个块了
                if(cur_off + LOB_PAGE_SIZE > data_off + len){//已经读到最后一个块了
                    state = 3;
                }
                else{
                    uint64_t offset;
                    create_lobpage(ll->get_seg_id(),offset,data+cur_off-data_off,LOB_PAGE_SIZE);
                    ll->append_lpas(offset,LOB_PAGE_SIZE);
                    cur_off += LOB_PAGE_SIZE;
                }
            }
            if(state == 3){//读到最后一个块了，只处理前半个部分
                uint8_t temp[item.second];
                read_lobpage(ll->get_seg_id(),item.first,temp,item.second);//读块
                uint64_t temp_off = data_off+len-cur_off;
                memcpy(temp,data+cur_off-data_off,temp_off);
                uint64_t offset;
                create_lobpage(ll->get_seg_id(),offset,temp,item.second);
                ll->append_lpas(offset,item.second);
                state = 4;
                continue;
            }
            if(state == 4){//已经结束，剩下的所有块依次处理
                ll->append_lpas(item.first,item.second);
            } 
        }
    }
    else if(ll->get_mode() == 0x12){
        uint64_t result_off = 0;
        uint64_t cur_off = 0;
        int state = 0;//表示还没找到位置，0表示找到了开始位置，1表示已经读写完毕
        LHP lhp, new_lhp;
        uint64_t lhpa;
        read_LHP(ll->get_seg_id(),ll->get_LHPA(),lhp);
        for(auto item:lhp.get_all_lpas()){
            if(state == 0){//还没找到位置，持续跳过
                if(cur_off + item.second > data_off){
                    state = 1;
                }
                else {
                    cur_off += item.second;
                    new_lhp.append(item.first,item.second);
                }
            }
            if(state == 1){//找到了第一个位置，处理第一个块的后半个部分
                uint64_t temp_off = data_off - cur_off;
                uint8_t temp[LOB_PAGE_SIZE];
                read_lobpage(ll->get_seg_id(),item.first,temp,item.second);
                if(temp_off + len < item.second){//如果本块内就够读取
                    memcpy(temp+temp_off,data,len);
                    uint64_t offset;
                    create_lobpage(ll->get_seg_id(),offset,temp,LOB_PAGE_SIZE);
                    new_lhp.append(offset,LOB_PAGE_SIZE);
                    cur_off += LOB_PAGE_SIZE;
                    state = 4;//结束
                    continue;
                }
                else {
                    memcpy(temp+temp_off,data,LOB_PAGE_SIZE-temp_off);//替换数据
                    uint64_t offset;
                    create_lobpage(ll->get_seg_id(),offset,temp,LOB_PAGE_SIZE);
                    new_lhp.append(offset,LOB_PAGE_SIZE);
                    cur_off += LOB_PAGE_SIZE;
                    state = 2;
                    continue;
                }
            }
            if(state == 2){//处在读取状态，连续读取，并判断是不是读到最后一个块了
                if(cur_off + LOB_PAGE_SIZE > data_off + len){//已经读到最后一个块了
                    state = 3;
                }
                else{
                    uint64_t offset;
                    create_lobpage(ll->get_seg_id(),offset,data+cur_off-data_off,LOB_PAGE_SIZE);
                    new_lhp.append(offset,LOB_PAGE_SIZE);
                    cur_off += LOB_PAGE_SIZE;
                }
            }
            if(state == 3){//读到最后一个块了，只处理前半个部分
                uint8_t temp[item.second];
                read_lobpage(ll->get_seg_id(),item.first,temp,item.second);//读块
                uint64_t temp_off = data_off+len-cur_off;
                memcpy(temp,data+cur_off-data_off,temp_off);
                uint64_t offset;
                create_lobpage(ll->get_seg_id(),offset,temp,item.second);
                new_lhp.append(offset,item.second);
                state = 4;
                continue;
            }
            if(state == 4){//已经结束，剩下的所有块依次处理
                new_lhp.append(item.first,item.second);
            } 
        }
        append_LHP(ll->get_seg_id(),lhpa,new_lhp);
        ll->set_LHPA(lhpa);

    }
    else if(ll->get_mode() == 0x13){
        uint64_t result_off = 0;
        uint64_t cur_off = 0;
        LHIP lhip,new_lhip;
        read_LHIP(ll->get_seg_id(),ll->get_LHIPA(),lhip);//首先读出来lhip，之后不覆盖写，重写
        int state = 0;//表示还没找到位置，0表示找到了开始位置，1表示已经读写完毕
        for(auto lhps:lhip.get_all_lhps()){
            LHP lhp, new_lhp;
            uint64_t lhpa;
            read_LHP(ll->get_seg_id(),lhps.first,lhp);
            for(auto item:lhp.get_all_lpas()){
                if(state == 0){//还没找到位置，持续跳过
                    if(cur_off + item.second > data_off){
                        state = 1;
                    }
                    else {
                        cur_off += item.second;
                        new_lhp.append(item.first,item.second);
                    }
                }
                if(state == 1){//找到了第一个位置，处理第一个块的后半个部分
                    uint64_t temp_off = data_off - cur_off;
                    uint8_t temp[LOB_PAGE_SIZE];
                    read_lobpage(ll->get_seg_id(),item.first,temp,item.second);
                    if(temp_off + len < item.second){//如果本块内就够读取
                        memcpy(temp+temp_off,data,len);
                        uint64_t offset;
                        create_lobpage(ll->get_seg_id(),offset,temp,LOB_PAGE_SIZE);
                        new_lhp.append(offset,LOB_PAGE_SIZE);
                        cur_off += LOB_PAGE_SIZE;
                        state = 4;//结束
                        continue;
                    }
                    else {
                        memcpy(temp+temp_off,data,LOB_PAGE_SIZE-temp_off);//替换数据
                        uint64_t offset;
                        create_lobpage(ll->get_seg_id(),offset,temp,LOB_PAGE_SIZE);
                        new_lhp.append(offset,LOB_PAGE_SIZE);
                        cur_off += LOB_PAGE_SIZE;
                        state = 2;
                        continue;
                    }
                }
                if(state == 2){//处在读取状态，连续读取，并判断是不是读到最后一个块了
                    if(cur_off + LOB_PAGE_SIZE > data_off + len){//已经读到最后一个块了
                        state = 3;
                    }
                    else{
                        uint64_t offset;
                        create_lobpage(ll->get_seg_id(),offset,data+cur_off-data_off,LOB_PAGE_SIZE);
                        new_lhp.append(offset,LOB_PAGE_SIZE);
                        cur_off += LOB_PAGE_SIZE;
                    }
                }
                if(state == 3){//读到最后一个块了，只处理前半个部分
                    uint8_t temp[item.second];
                    read_lobpage(ll->get_seg_id(),item.first,temp,item.second);//读块
                    uint64_t temp_off = data_off+len-cur_off;
                    memcpy(temp,data+cur_off-data_off,temp_off);
                    uint64_t offset;
                    create_lobpage(ll->get_seg_id(),offset,temp,item.second);
                    new_lhp.append(offset,item.second);
                    state = 4;
                    continue;
                }
                if(state == 4){//已经结束，剩下的所有块依次处理
                    new_lhp.append(item.first,item.second);
                } 
            }
            append_LHP(ll->get_seg_id(),lhpa,new_lhp);
            new_lhip.append(lhpa,new_lhp.data_size);
        }
        uint64_t lhipa;
        append_LHIP(ll->get_seg_id(),lhipa,new_lhip);//写新的lhip
        ll->set_LHIPA(lhipa);
    }


    if(modify_len <= data_off + len){//如果还有部分没有搞定,剩余的以append形式处理
        append(ll,data+modify_len,data_off+len-modify_len);
    }
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
        uint64_t cur_off = 0;//用来表示当前的大小
        LHP lhp;
        read_LHP(ll->get_seg_id(),ll->get_LHPA(),lhp);//读取lhp出来
        int state = 0;
        for(auto item:lhp.get_all_lpas()){
            if(state == 1){//说明已经应该读块了
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
                if(cur_off + item.second > data_off){//说明当前块就是了
                    //读取这个块的一部分
                    state = true;
                    uint8_t temp[item.second];
                    uint64_t temp_off = data_off - cur_off;
                    read_lobpage(ll->get_seg_id(),item.first,temp,item.second);//读这一块
                    if(item.second - temp_off >= amount){//说明要读的数量很小，就在这一块
                        memcpy(result+result_off,temp+temp_off,amount);
                        break;
                    }
                    else{
                        memcpy(result + result_off,temp+temp_off,item.second-temp_off);
                        state = 1;
                    }
                    result_off += item.second - temp_off;
                }
                else{
                    cur_off += item.second;
                }
            }
        }
        
    }
    else if(ll->get_mode() == 0x13){
        uint64_t result_off = 0;
        uint64_t cur_off = 0;//用来表示当前的大小
        LHIP lhip;
        read_LHIP(ll->get_seg_id(),ll->get_LHIPA(),lhip);//读出索引
        int state = 0;//表示是否找到了应该开始读的位置
        for(auto lhps:lhip.get_all_lhps()){
            LHP lhp;
            read_LHP(ll->get_seg_id(),lhps.first,lhp);//读取出LHP
            for(auto item:lhp.get_all_lpas()){
                if(state == 1){//说明已经应该读块了
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
                    if(cur_off + item.second >= data_off){//说明当前块就是了
                        //读取这个块的一部分
                        state = true;
                        uint8_t temp[item.second];
                        uint64_t temp_off = data_off - cur_off;
                        read_lobpage(ll->get_seg_id(),item.first,temp,item.second);//读这一块
                        if(item.second - temp_off > amount){//说明要读的数量很小，就在这一块
                            memcpy(result+result_off,temp+temp_off,amount);
                            break;
                        }
                        else{
                            memcpy(result + result_off,temp+temp_off,item.second-temp_off);
                            state = 1;
                        }
                        result_off += item.second - temp_off;
                    }
                    else{
                        cur_off += item.second;
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

