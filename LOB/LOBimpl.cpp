#include "LOB_locator.h"
#include "LOB.h"

Status LOBLocator::Serialize(std::vector<uint8_t> &result, LOBLocator **ll){
    ;
}
Status LOBLocator::Deserialize(const std::vector<uint8_t> &input, LOBLocator *ll){
    ;
}

LOBimpl::LOBimpl(){
    ;
}

uint32_t LOBimpl::new_lob_id(){
    return 1001;
}

Status LOBimpl::create_locator(LOBLocator **llp, uint32_t seg_id){
    Status s;
    uint32_t lob_new_id =  new_lob_id();//创建一个LOBID
    *llp = new LOBLocator(lob_new_id,seg_id,0x01);
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
    uint64_t newlen = data.size() + ll->data_size;
    if(newlen > LOBimpl::OUTLINE_3_MAX_SIZE){
        s.FetalError("插入的数据超过最大限制");
        return s;
    }
    if(ll->mode == 0x10){//初始行内
        if(newlen <= LOBLocator::INLINE_MAX_SIZE){//继续行内存
            ll->data.insert(ll->data.end(),data.begin(),data.end());//直接插入数据
            ll->update_head(ll->size + data.size(),0x10,ll->data_size+data.size(),checksum(ll));//更新头部
            return s;
        }
        else{
            vector<uint8_t> temp_data = ll->data;
            vector<uint8_t>().swap(ll->data);//置空data
            ll->lpas.clear();
            temp_data.insert(temp_data.end(),data.begin(),data.end());
            uint32_t i = 0,beg = 0;
            uint64_t lpa,lhpa,lhipa;
            LHP *lhp;
            LHIP *lhip;
            for(;beg < temp_data.size(); i++,beg+=LOB_PAGE_SIZE){
                if(ll->lpa_nums < LOBLocator::MAX_LPA){
                    create_lobpage(ll->segID,lpa,data,beg,LOB_PAGE_SIZE);
                    ll->lpas.emplace_back(lpa);
                    ll->lpa_nums++;
                }
                else{
                    create_LHP(&lhp);
                    for(int i =0; i < LOBLocator::MAX_LPA; i++){
                        lhp->append(ll->lpas[i]);
                    }
                    break;
                }
            }
            if(beg >= temp_data.size()){//说明不用一级索引
                ll->update_head(LOBLocator::HEAD_SIZE + 4 + ll->lpa_nums * 4,0x11,ll->data_size+data.size(),checksum(ll));
                return s;
            }
            for(;beg < temp_data.size(); i++,beg+=LOB_PAGE_SIZE){
                if(lhp->nums < LHP::MAX_LPA){//一个LHP勉强还够
                    create_lobpage(ll->segID,lpa,data,beg,LOB_PAGE_SIZE);
                    lhp->append(lpa);
                }
                else{
                    create_LHIP(&lhip);
                    write_LHP(ll->segID,lhpa,lhp);
                    lhip->append(lhpa);
                    delete lhp;
                    create_LHP(&lhp);
                    break;
                }
            }
            if(beg >= temp_data.size()){//已经超过了,说明没用二级索引
                write_LHP(ll->segID,lhpa,lhp);
                ll->lhpa = lhpa;
                ll->update_head(LOBLocator::HEAD_SIZE + 8 + ll->lpa_nums,0x12,ll->data_size+data.size(),checksum(ll));
                delete lhp;
                return s;
            }
            create_LHP(&lhp);//再重新创建一个lhp
            for(;beg < temp_data.size(); i++,beg+=LOB_PAGE_SIZE){
                if(lhp->nums < LHP::MAX_LPA){
                    create_lobpage(ll->segID,lpa,data,beg,LOB_PAGE_SIZE);
                    lhp->append(lpa);
                }
                else{
                    write_LHP(ll->segID,lhpa,lhp);
                    lhip->append(lhpa);
                    delete lhp;
                    create_LHP(&lhp);
                }
            }
            write_LHP(ll->segID,lhpa,lhp);
            lhip->append(lhpa);
            write_LHIP(ll->segID,lhipa,lhip);
            ll->lhpa = lhipa;          
            delete lhp;
            delete lhip;
            ll->update_head(LOBLocator::HEAD_SIZE + 8 + ll->lpa_nums,0x13,ll->data_size+data.size(),checksum(ll));
            return s;
        }
    }
    else if(ll->mode == 0x11){
        //首先处理最后一个块不满的问题
        uint64_t last = LOB_PAGE_SIZE - (ll->data_size % LOB_PAGE_SIZE);
        uint64_t last_lpa = ll->lpas[ll->lpa_nums-1];
        vector<uint8_t> output;
        read_lobpage(ll->segID,last_lpa,output);
        output.insert(output.end(),data.begin(),data.begin()+last);
        write_lobpage(ll->segID,last_lpa,output,0,output.size());
        //然后开始正常操作
        uint64_t i = ll->lpa_nums,beg = last;
        uint64_t lpa,lhpa,lhipa;
        LHP *lhp;
        LHIP *lhip;
        for(;beg < data.size(); i++,beg+=LOB_PAGE_SIZE){
            if(ll->lpa_nums < LOBLocator::MAX_LPA){
                create_lobpage(ll->segID,lpa,data,beg,LOB_PAGE_SIZE);
                ll->lpas.emplace_back(lpa);
                ll->lpa_nums++;
            }
            else{
                create_LHP(&lhp);
                for(int i =0; i < LOBLocator::MAX_LPA; i++){
                    lhp->append(ll->lpas[i]);
                }
                break;
            }
        }
        if(beg >= data.size()){//说明不用一级索引
            ll->update_head(LOBLocator::HEAD_SIZE + 4 + ll->lpa_nums * 4,0x11,ll->data_size+data.size(),checksum(ll));
            return s;
        }
        for(;beg < data.size(); i++,beg+=LOB_PAGE_SIZE){
            if(lhp->nums < LHP::MAX_LPA){//一个LHP勉强还够
                create_lobpage(ll->segID,lpa,data,beg,LOB_PAGE_SIZE);
                lhp->append(lpa);
            }
            else{
                create_LHIP(&lhip);
                write_LHP(ll->segID,lhpa,lhp);
                lhip->append(lhpa);
                delete lhp;
                create_LHP(&lhp);
                break;
            }
        }
        if(beg >= data.size()){//已经超过了,说明没用二级索引
            write_LHP(ll->segID,lhpa,lhp);
            ll->lhpa = lhpa;
            ll->update_head(LOBLocator::HEAD_SIZE + 8 + ll->lpa_nums,0x12,ll->data_size+data.size(),checksum(ll));
            delete lhp;
            return s;
        }
        create_LHP(&lhp);//再重新创建一个lhp
        for(;beg < data.size(); i++,beg+=LOB_PAGE_SIZE){
            if(lhp->nums < LHP::MAX_LPA){
                create_lobpage(ll->segID,lpa,data,beg,LOB_PAGE_SIZE);
                lhp->append(lpa);
            }
            else{
                write_LHP(ll->segID,lhpa,lhp);
                lhip->append(lhpa);
                delete lhp;
                create_LHP(&lhp);
            }
        }
        write_LHP(ll->segID,lhpa,lhp);
        lhip->append(lhpa);
        write_LHIP(ll->segID,lhipa,lhip);
        ll->lhpa = lhipa;          
        delete lhp;
        delete lhip;
        ll->update_head(LOBLocator::HEAD_SIZE + 8 + ll->lpa_nums,0x13,ll->data_size+data.size(),checksum(ll));
        return s;
    }
    else if(ll->mode == 0x12){
        //首先处理最后一个块不满的问题
        uint64_t last = LOB_PAGE_SIZE - (ll->data_size % LOB_PAGE_SIZE);
        uint64_t lpa_all_nums = (ll->data_size / LOB_PAGE_SIZE) + 1;
        LHP *lhp;
        read_LHP(ll->segID,ll->lhpa,&lhp);//读出lhp结构来
        uint64_t last_lpa = lhp->M[lpa_all_nums - 1];
        vector<uint8_t> output;
        read_lobpage(ll->segID,last_lpa,output);
        output.insert(output.end(),data.begin(),data.begin()+last);
        write_lobpage(ll->segID,last_lpa,output,0,output.size());
        //然后开始正常操作
        uint64_t i = lhp->nums,beg = last;
        uint64_t lpa,lhpa,lhipa;
        LHIP *lhip;
        for(;beg < data.size(); i++,beg+=LOB_PAGE_SIZE){
            if(lhp->nums < LHP::MAX_LPA){//一个LHP勉强还够
                create_lobpage(ll->segID,lpa,data,beg,LOB_PAGE_SIZE);
                lhp->append(lpa);
            }
            else{
                create_LHIP(&lhip);
                write_LHP(ll->segID,lhpa,lhp);
                lhip->append(lhpa);
                delete lhp;
                create_LHP(&lhp);
                break;
            }
        }
        if(beg >= data.size()){//已经超过了,说明没用二级索引
            write_LHP(ll->segID,lhpa,lhp);
            ll->lhpa = lhpa;
            ll->update_head(LOBLocator::HEAD_SIZE + 8 + ll->lpa_nums,0x12,ll->data_size+data.size(),checksum(ll));
            delete lhp;
            return s;
        }
        create_LHP(&lhp);//再重新创建一个lhp
        for(;beg < data.size(); i++,beg+=LOB_PAGE_SIZE){
            if(lhp->nums < LHP::MAX_LPA){
                create_lobpage(ll->segID,lpa,data,beg,LOB_PAGE_SIZE);
                lhp->append(lpa);
            }
            else{
                write_LHP(ll->segID,lhpa,lhp);
                lhip->append(lhpa);
                delete lhp;
                create_LHP(&lhp);
            }
        }
        write_LHP(ll->segID,lhpa,lhp);
        lhip->append(lhpa);
        write_LHIP(ll->segID,lhipa,lhip);
        ll->lhpa = lhipa;
        ll->update_head(LOBLocator::HEAD_SIZE + 8 + ll->lpa_nums,0x13,ll->data_size+data.size(),checksum(ll));
        delete lhip;
        return s;
    }
    else if(ll->mode == 0x13){
       //首先处理最后一个块不满的问题
        uint64_t last = LOB_PAGE_SIZE - (ll->data_size % LOB_PAGE_SIZE);
        uint64_t lpa_all_nums = (ll->data_size / LOB_PAGE_SIZE) + 1;
        LHIP *lhip;
        read_LHIP(ll->segID,ll->lhpa,&lhip);
        LHP* lhp;
        read_LHP(ll->segID,lhip->read_last_lhpa(),&lhp);//读出lhp结构来
        uint64_t last_lpa = lhp->M[lpa_all_nums];
        vector<uint8_t> output;
        read_lobpage(ll->segID,last_lpa,output);
        output.insert(output.end(),data.begin(),data.begin()+last);
        write_lobpage(ll->segID,last_lpa,output,0,LOB_PAGE_SIZE);
        //然后正常操作
        uint64_t i = lhip->nums,beg = last;
        uint64_t lpa,lhpa,lhipa;
        create_LHP(&lhp);
        for(;beg < data.size(); i++,beg+=LOB_PAGE_SIZE){
            if(lhp->nums < LHP::MAX_LPA){
                create_lobpage(ll->segID,lpa,data,beg,LOB_PAGE_SIZE);
                lhp->append(lpa);
            }
            else{
                write_LHP(ll->segID,lhpa,lhp);
                lhip->append(lhpa);
                delete lhp;
                create_LHP(&lhp);
            }
        }
        write_LHP(ll->segID,lhpa,lhp);
        lhip->append(lhpa);
        write_LHIP(ll->segID,lhipa,lhip);
        ll->lhpa = lhipa;
        ll->update_head(LOBLocator::HEAD_SIZE + 8 + ll->lpa_nums,0x13,ll->data_size+data.size(),checksum(ll));
        delete lhip;
        return s;
    }
    else{
        s.FetalError("模式错误");
        return s;
    }
    
}
Status LOBimpl::write(LOBLocator *ll, uint64_t data_off, const uint8_t *data){
    Status s;
    if(data_off > ll->data_size) s.FetalError("插入数据的位置不应该大于数据本身大小");
    if(data_off == ll->data_size) append(ll,data);
    if(ll->mode == 0x10){//初始行内
        vector<uint8_t> temp;
        temp.insert(temp.end(),ll->data.begin()+data_off,ll->data.end());
        
    }
    else if(ll->mode == 0x11){
        
    }
    else if(ll->mode == 0x12){

    } 
    else if(ll->mode == 0x13){

    }
    else{
        s.FetalError("模式错误");
    }
    return s;
}
Status LOBimpl::read(LOBLocator *ll, uint64_t amount, uint64_t data_off, uint8_t *result){
    Status s;
    
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
    if(data.size() - beg < len) len = data.size() - beg;
    LOBH lobh(len,checksum_data(data,beg,len));
    vector<uint8_t> tdta;
    lobh.Serialize(tdta);
    tdta.insert(tdta.end(),data.begin() + beg,data.begin()+beg + len);
    s = vfs->append(segid,offset,tdta,0,tdta.size());
    return s;
}

Status LOBimpl::write_lobpage(uint32_t segid, uint64_t offset, const uint8_t *data, uint64_t len){
    Status s;
    VFS *vfs = VFS::get_VFS();
    if(data.size() - beg < len) len = data.size() - beg;
    LOBH lobh(len,checksum_data(data,beg,len));
    vector<uint8_t> tdta;
    lobh.Serialize(tdta);
    tdta.insert(tdta.end(),data.begin() + beg,data.begin()+beg + len);
    s = vfs->write_page(segid,offset,tdta,0,tdta.size());
    return s;
}
Status LOBimpl::read_lobpage(uint32_t segid, uint64_t offset, uint8_t *output){
    Status s;
    VFS *vfs = VFS::get_VFS();
    vector<uint8_t> temp;
    s = vfs->read_page(segid,offset,temp);
    output.insert(output.end(),temp.begin()+LOBH::HEAD_SIZE,temp.end());
    return s;
}
Status LOBimpl::create_LHP(LHP **lhp){
    Status s;
    *lhp = new LHP();
    return s;
}
//Status LOBimpl::append_LHP(uint32_t segid, uint32_t &offset, LHP *lhp){

//}
Status LOBimpl::write_LHP(uint32_t segid, uint64_t &offset, LHP *lhp){
    Status s;
    VFS *vfs = VFS::get_VFS();
    vector<uint8_t> result;
    lhp->Serialize(result);
    s = vfs->append(segid,offset,result,0,result.size());
    return s;
}
Status LOBimpl::read_LHP(uint32_t segid, uint64_t offset, LHP **lhp){
    Status s;
    VFS *vfs = VFS::get_VFS();
    vector<uint8_t> data;
    vfs->read_page(segid,offset,data);
    *lhp = new LHP();
    (*lhp) -> Deserialize(data);
    return s;
}
Status LOBimpl::create_LHIP(LHIP **lhip){
    Status s;
    *lhip = new LHIP();
    return s;
}

Status LOBimpl::write_LHIP(uint32_t segid, uint64_t &offset, LHIP *lhip){
    Status s;
    VFS *vfs = VFS::get_VFS();
    vector<uint8_t> result;
    lhip->Serialize(result);
    s = vfs->append(segid,offset,result,0,result.size());
    return s;
}
Status LOBimpl::read_LHIP(uint32_t segid, uint64_t offset, LHIP **lhip){
    Status s;
    VFS *vfs = VFS::get_VFS();
    vector<uint8_t> data;
    vfs->read_page(segid,offset,data);
    *lhip = new LHIP();
    (*lhip) -> Deserialize(data);
    return s;
}

