#include "LOB_locator.h"
#include "LOB.h"
Status LOBLocator::Serialize(std::vector<uint8_t> &result, LOBLocator **ll){
    ;
}
Status LOBLocator::Deserialize(const std::vector<uint8_t> &input, LOBLocator *ll){
    ;
}

Status LOB::append(LOBLocator *ll, const std::vector<uint8_t> &data){
    Status s;
    uint32_t newlen = data.size() + ll->data_size;
    if(ll->mode == 0x10){//初始行内存
        if(newlen <= LOBLocator::INLINE_MAX_SIZE){//继续行内存
            ll->data_size += data.size();
            ll->H.data.insert(ll->H.data.end(),data.begin(),data.end());//直接插入数据
            ll->LOB_checksum = checksum(ll);
            return s;
        }
        else{//否则是行外存，先初始化一个段
            Segment *sg;
            Segment::create_seg(ll->LOBID,df,op,&sg);
            if(newlen <= OUTLINE_1_MAX_SIZE){//可以在行内放下指示
                ll->H.data.insert(ll->H.data.end(),data.begin(),data.end());//临时先加进来
                int nums = ll->H.data.size()/LOB_PAGE_SIZE;
                for(int i = 0; i < nums; i++){
                    
                }
            }
            else if(newlen <= OUTLINE_2_MAX_SIZE){

            }
            else if(newlen<=OUTLINE_3_MAX_SIZE){

            }
            else{
                s.FetalError("插入的数据超过最大限制");
                return s
            }
        }
    }
    else if(ll->mode == 0x11){//初始行外存1

    }
    else if(ll->mode == 0x12){

    }
    else if(ll->mode == 0x13){

    }
    else{
        s.FetalError("模式错误");
        return s;
    }
    
}
Status LOB::write(LOBLocator *ll, uint32_t offset, const std::vector<uint8_t> &data){
    
}
Status LOB::read(LOBLocator *ll, uint32_t amount, uint32_t offset, std::vector<uint8_t> &result){
    
}
Status LOB::erase(LOBLocator *ll, uint32_t amount, uint32_t offset){

}
Status LOB::drop(LOBLocator *ll){
    
}
uint32_t LOB::checksum(LOBLocator *ll){
    return 0x01;
}

Status LOB::

