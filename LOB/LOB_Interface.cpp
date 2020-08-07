#include "LOB.h"

Status LOB::COMPARE(LOBLocator *lob_1,LOBLocator *lob_2,uint64_t amount, uint64_t offset_1, uint64_t offset_2){

}

Status LOB::APPEND(LOBLocator *dest_lob, LOBLocator src_lob){

}

Status LOB::COPY(LOBLocator *dest_lob,LOBLocator *src_lob,uint64_t amount, uint64_t dest_offset, uint64_t src_offset){

}

Status LOB::ERASE(LOBLocator *lob_loc, uint64_t amount, uint64_t offset){

}

Status LOB::FRAGMENT_DELETE(LOBLocator *lob_loc, uint64_t amount, uint64_t offset){

}


Status LOB::FRAGMENT_INSERT(LOBLocator *lob_loc, uint64_t amount, uint64_t offset, const uint8_t *data){

}

Status LOB::FRAGMENT_MOVE(LOBLocator *lob_loc, uint64_t amount, uint64_t src_offset, uint64_t dest_offset){

}

Status LOB::FRAGMENT_REPLACE(LOBLocator *lob_loc, uint64_t old_amount, uint64_t new_amount, uint64_t offset, const uint8_t *data){

}


Status LOB::READ(LOBLocator *lob_loc,uint64_t amount,uint64_t offset, uint8_t *data){

}

Status LOB::WRITE(LOBLocator *lob_loc,uint64_t amount,uint64_t offset, const uint8_t *data){

}

Status LOB::WRITEAPPEND(LOBLocator *lob_loc, uint64_t amount, const uint8_t *data){

}

Status LOB::TRIM(LOBLocator *lob_loc, uint64_t newlen){

}