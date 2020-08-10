#include "LOB.h"

bool LOB::COMPARE(LOBLocator *lob_1,LOBLocator *lob_2,uint64_t amount, uint64_t offset_1, uint64_t offset_2){
    uint8_t lb1[amount],lb2[amount];
    LOBimpl lob;
    lob.read(lob_1,offset_1,lb1,amount);
    lob.read(lob_2,offset_2,lb2,amount);
    for(int i = 0; i < amount; i++){
        if(lb1[i] != lb2[i]) return false;
    }
    return true;
}

Status LOB::APPEND(LOBLocator *dest_lob, LOBLocator *src_lob){
    Status s;
    const uint64_t MAX_SIZE = 100000;//一次最多拷贝这么多
    LOBimpl lob;
    if(src_lob->get_data_size() < MAX_SIZE){
        uint8_t temp[src_lob->get_data_size()];
        lob.read(src_lob,0,temp,src_lob->get_data_size());
        lob.append(dest_lob,temp,src_lob->get_data_size());
    }
    else{
        uint64_t off = 0;
        for(;off + MAX_SIZE < src_lob->get_data_size();off += MAX_SIZE){
            uint8_t temp[MAX_SIZE];
            lob.read(src_lob,off,temp,MAX_SIZE);
            lob.append(dest_lob,temp,MAX_SIZE);
        }
        uint8_t temp[src_lob->get_data_size() - off];
        lob.read(src_lob,off,temp,src_lob->get_data_size() - off);
        lob.append(dest_lob,temp,src_lob->get_data_size() - off);
    }
    return s;
}

Status LOB::COPY(LOBLocator *dest_lob,LOBLocator *src_lob,uint64_t amount, uint64_t dest_offset, uint64_t src_offset){

}

Status LOB::ERASE(LOBLocator *lob_loc, uint64_t amount, uint64_t offset){
    Status s;
    LOBimpl lob;
    lob.erase(lob_loc,offset,amount);
    return s;
}

Status LOB::FRAGMENT_DELETE(LOBLocator *lob_loc, uint64_t amount, uint64_t offset){
    Status s;
    LOBimpl lob;
    lob.erase(lob_loc,offset,amount);
    return s;
}


Status LOB::FRAGMENT_INSERT(LOBLocator *lob_loc, uint64_t amount, uint64_t offset, const uint8_t *data){

}

Status LOB::FRAGMENT_MOVE(LOBLocator *lob_loc, uint64_t amount, uint64_t src_offset, uint64_t dest_offset){

}

Status LOB::FRAGMENT_REPLACE(LOBLocator *lob_loc, uint64_t old_amount, uint64_t new_amount, uint64_t offset, const uint8_t *data){
    Status s;
    
    return s;
}


Status LOB::READ(LOBLocator *lob_loc,uint64_t amount,uint64_t offset, uint8_t *data){

}

Status LOB::WRITE(LOBLocator *lob_loc,uint64_t amount,uint64_t offset, const uint8_t *data){

}

Status LOB::WRITEAPPEND(LOBLocator *lob_loc, uint64_t amount, const uint8_t *data){

}

Status LOB::TRIM(LOBLocator *lob_loc, uint64_t newlen){

}