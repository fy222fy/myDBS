#include "LOB.h"
uint32_t LOB::new_lob_id(){
    return 1001;
}
Status LOB::create_locator(LOBLocator *llp, uint32_t seg_id){
    Status s;
    uint32_t lob_new_id = new_lob_id();//创建一个LOBID
    llp->init(lob_new_id,seg_id,0x01);
    return s;
}

Status LOB::create_lobseg(uint32_t &seg_id){
    Status s;
    VFS *vfs = VFS::get_VFS();//获取VFS
    seg_id = vfs->new_segid();
    vfs->create_seg(seg_id);
    return s;
}


bool LOB::COMPARE(LOBLocator *lob_1,LOBLocator *lob_2,uint64_t amount, uint64_t offset_1, uint64_t offset_2){
    uint8_t lb1[amount],lb2[amount];
    LOBimpl lob1(lob_1);
    LOBimpl lob2(lob_2);
    lob1.read(offset_1,lb1,amount);
    lob2.read(offset_2,lb2,amount);
    for(int i = 0; i < amount; i++){
        if(lb1[i] != lb2[i]) return false;
    }
    return true;
}

Status LOB::APPEND(LOBLocator *dest_lob, LOBLocator *src_lob){
    Status s;
    const uint64_t MAX_SIZE = 100000;//一次最多拷贝这么多
    LOBimpl dest(dest_lob);
    LOBimpl src(src_lob);
    if(src_lob->get_data_size() < MAX_SIZE){
        uint8_t temp[src_lob->get_data_size()];
        src.read(0,temp,src_lob->get_data_size());
        dest.append(temp,src_lob->get_data_size());
        dest.finish();
    }
    else{
        uint64_t off = 0;
        for(;off + MAX_SIZE < src_lob->get_data_size();off += MAX_SIZE){
            uint8_t temp[MAX_SIZE];
            src.read(off,temp,MAX_SIZE);
            dest.append(temp,MAX_SIZE);
        }
        uint8_t temp[src_lob->get_data_size() - off];
        src.read(off,temp,src_lob->get_data_size() - off);
        dest.append(temp,src_lob->get_data_size() - off);
    }
    return s;
}

Status LOB::COPY(LOBLocator *dest_lob,LOBLocator *src_lob,uint64_t amount, uint64_t dest_offset, uint64_t src_offset){
    Status s;
    LOBimpl dest(dest_lob);
    LOBimpl src(src_lob);
    dest.erase(dest_offset,amount);
    uint8_t temp[amount];
    src.read(src_offset,temp,amount);
    dest.insert(dest_offset,temp,amount);
    dest.finish();
    return s;
}

Status LOB::ERASE(LOBLocator *lob_loc, uint64_t amount, uint64_t offset){
    Status s;
    LOBimpl lob(lob_loc);
    lob.erase(offset,amount);
    lob.finish();
    return s;
}

Status LOB::FRAGMENT_DELETE(LOBLocator *lob_loc, uint64_t amount, uint64_t offset){
    Status s;
    LOBimpl lob(lob_loc);
    lob.erase(offset,amount);
    lob.finish();
    return s;
}


Status LOB::FRAGMENT_INSERT(LOBLocator *lob_loc, uint64_t amount, uint64_t offset, const uint8_t *data){
    Status s;
    LOBimpl lob(lob_loc);
    lob.insert(offset,data,amount);
    lob.finish();
    return s;
}

Status LOB::FRAGMENT_MOVE(LOBLocator *lob_loc, uint64_t amount, uint64_t src_offset, uint64_t dest_offset){
    Status s;
    LOBimpl lob(lob_loc);
    uint8_t temp[amount];
    lob.read(src_offset,temp,amount);//先读出来
    lob.erase(dest_offset,amount);
    lob.insert(dest_offset,temp,amount);
    lob.finish();
    return s;
}

Status LOB::FRAGMENT_REPLACE(LOBLocator *lob_loc, uint64_t old_amount, uint64_t new_amount, uint64_t offset, const uint8_t *data){
    Status s;
    LOBimpl lob(lob_loc);
    lob.erase(offset,old_amount);
    lob.insert(offset,data,new_amount);
    lob.finish();
    return s;

}


Status LOB::READ(LOBLocator *lob_loc,uint64_t amount, uint64_t offset, uint8_t *data){
    Status s;
    LOBimpl lob(lob_loc);
    lob.read(offset,data,amount);
    return s;
}


Status LOB::WRITEAPPEND(LOBLocator *lob_loc, uint64_t amount, const uint8_t *data){
    Status s;
    LOBimpl lob(lob_loc);
    lob.append(data,amount);
    lob.finish();
    return s;
}

Status LOB::TRIM(LOBLocator *lob_loc, uint64_t newlen){
    Status s;
    if(newlen < lob_loc->get_data_size()) s.FetalError("裁剪后的长度不能大于裁剪前的长度");
    LOBimpl lob(lob_loc);
    lob.erase(newlen,lob_loc->get_data_size());
    lob.finish();
    return s;
}

