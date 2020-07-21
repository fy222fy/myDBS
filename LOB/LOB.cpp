#include "LOB_locator.h"
#include "LOB.h"

Status LOBLocator::Serialize(std::vector<uint8_t> &result, LOBLocator **ll){
    ;
}
Status LOBLocator::Deserialize(const std::vector<uint8_t> &input, LOBLocator *ll){
    ;
}

LOB::LOB(){
    ;
}

uint32_t LOB::new_lob_id(){
    return 1001;
}

Status LOB::create(LOBLocator **llp){
    Status s;
    uint32_t lob_new_id =  new_lob_id();
    *llp = new LOBLocator(lob_new_id,0x01);
    return s;
}

Status LOB::append(LOBLocator *ll, const std::vector<uint8_t> &data){
    Status s;
    uint32_t newlen = data.size() + ll->data_size;
    if(newlen > LOB::OUTLINE_3_MAX_SIZE){
        s.FetalError("插入的数据超过最大限制");
        return s;
    }
    if(ll->mode == 0x10){//初始行内存
        if(newlen <= LOBLocator::INLINE_MAX_SIZE){//继续行内存
            ll->data_size += data.size();
            ll->H.data.insert(ll->H.data.end(),data.begin(),data.end());//直接插入数据
            ll->LOB_checksum = checksum(ll);
            return s;
        }
        else if(newlen <= OUTLINE_1_MAX_SIZE){
            int lpa_inline_nums = (data.size() + ll->data_size) / LOB_PAGE_SIZE;//要化为多少个lpa
            vector<uint8_t> temp_data = ll->H.data;
            temp_data.insert(temp_data.end(),data.begin(),data.end());
            ll->H.tail0.lpas.clear();
            ll->H.tail0.nums = lpa_inline_nums;
            uint32_t beg = 0;
            uint32_t lpa;
            for(int i = 0; i < lpa_inline_nums; i++){
                create_lobpage(lpa,temp_data,beg,LOB_PAGE_SIZE);
                beg += LOB_PAGE_SIZE;
                ll->H.tail0.lpas.emplace_back(lpa);
            }
        }
        else if(newlen <= OUTLINE_2_MAX_SIZE){
            LHP *lhp;
            create_LHP(&lhp);
            int lpa_nums = (data.size() + ll->data_size) / LOB_PAGE_SIZE;
            vector<uint8_t> temp_data = ll->H.data;
            temp_data.insert(temp_data.end(),data.begin(),data.end());
            uint32_t beg = 0;
            uint32_t lpa;
            for(int i = 0; i < lpa_nums; i++){
                create_lobpage(lpa,temp_data,beg);
                beg += LOB_PAGE_SIZE;
                lhp.append(lpa);
            }
            uint32_t offset;
            write_LHP(offset,lhp);
        }
        else if(newlen <= LOB::OUTLINE_3_MAX_SIZE){
            
        }
    }
    else if(ll->mode == 0x12){
        if(newlen <= LOB::OUTLINE_2_MAX_SIZE){//2模式没超过
            
        }
        else if(newlen <= LOB::OUTLINE_3_MAX_SIZE){
        
        }
    }
    else if(ll->mode == 0x13){
       
    }
    else{
        s.FetalError("模式错误");
        return s;
    }
    
}
Status LOB::write(LOBLocator *ll, uint32_t offset, const std::vector<uint8_t> &data){
    Status s;
    return s;
}
Status LOB::read(LOBLocator *ll, uint32_t amount, uint32_t offset, std::vector<uint8_t> &result){
    Status s;
    return s;
}
Status LOB::erase(LOBLocator *ll, uint32_t amount, uint32_t offset){
    Status s;
    return s;
}
Status LOB::drop(LOBLocator *ll){
    Status s;
    return s;
}
///////////////////////private function///////////////////////

uint32_t LOB::checksum(LOBLocator *ll){
    return 0x01;
}

Status LOB::create_lobpage(uint32_t &offset, const vector<uint8_t> &data, uint32_t beg, uint32_t len){
    
}
Status LOB::append_lobpage(uint32_t &offset, LOBP *lobp){

}
Status LOB::write_lobpage(uint32_t offset, LOBP *lobp){

}
Status LOB::read_lobpage(uint32_t offset, LOBP **lobp){

}
Status LOB::create_LHP(LHP **lhp){

}
Status LOB::append_LHP(uint32_t &offset, LHP *lhp){

}
Status LOB::write_LHP(uint32_t offset, LHP *lhp){

}
Status LOB::read_LHP(uint32_t offset, LHP **lhp){

}
Status LOB::create_LHIP(LHIP **lhip){

}
Status LOB::appned_LHIP(uint32_t &offset, LHIP *lhip){

}
Status LOB::write_LHIP(uint32_t offset, LHIP *lhip){

}
Status LOB::read_LHIP(uint32_t offset, LHIP **lhip){
    
}

