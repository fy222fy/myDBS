#include "LOB_locator.h"
#include "LOB.h"


Status LOBLocator::Serialize(std::vector<uint8_t> &result, LOBLocator **ll){
    ;
}
Status LOBLocator::Deserialize(const std::vector<uint8_t> &input, LOBLocator *ll){
    ;
}

LOB::LOB(DataFile* df, Options *op):df(df),op(op){

}

Status LOB::create(LOBLocator **llp, uint32_t seg_id){
    uint32_t lob_new_id =  seg_id;//如果要第二个LOB，这里要增长
    *llp = new LOBLocator(lob_new_id,0x01);
    
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
                vector<uint8_t> temp = ll->H.data;//拷贝
                uint32_t last = LOB_PAGE_SIZE - temp.size();//缺的长度
                temp.insert(temp.end(),data.begin(),data.begin()+last);
                uint32_t offset;
                create_lobpage(offset,temp,0);
                ll->H.tail0.nums = 1;
                ll->H.tail0.lpas.emplace_back(offset);
                int nums = (int) (data.size()-last)/LOB_PAGE_SIZE;
                int beg = last;//补足的开始点
                while(nums-->0){
                    create_lobpage(offset,data,beg);
                    ll->H.tail0.nums ++;
                    ll->H.tail0.lpas.emplace_back(offset);
                    beg += LOB_PAGE_SIZE;
                }
            }
            else if(newlen <= OUTLINE_2_MAX_SIZE){//二级方案
                vector<uint8_t> temp = ll->H.data;//拷贝
                uint32_t last = LOB_PAGE_SIZE - temp.size();//缺的长度
                temp.insert(temp.end(),data.begin(),data.begin()+last);
                uint32_t lhpa;
                create_LHP(lhpa);//创建一个LHP
                ll->H.tail1.lhpa = lhpa;
                uint32_t offset;
                create_lobpage(offset,temp,0);
                append_LHP(lhpa,offset);//写入第一个地址
                int nums = (int) (data.size()-last)/LOB_PAGE_SIZE;
                int beg = last;
                int inrow_nums = 6;
                ll->H.tail1.nums = 6;
                while(nums-->0){
                    create_lobpage(offset,data,beg);
                    beg += LOB_PAGE_SIZE;
                    append_LHP(lhpa,offset);
                    if(inrow_nums > 0){
                        ll->H.tail1.lpas.emplace_back(offset);
                    }
                }
            }
            else if(newlen<=OUTLINE_3_MAX_SIZE){
                vector<uint8_t> temp = ll->H.data;//拷贝
                uint32_t last = LOB_PAGE_SIZE - temp.size();//缺的长度
                temp.insert(temp.end(),data.begin(),data.begin()+last);
                uint32_t lhpia;
                create_LHPI(lhpia);
                ll->H.tail2.lhpia = lhpia;
                ll->H.tail2.nums = 6;
                uint32_t offset;
                create_lobpage(offset,temp,0);
                uint32_t lhp_nums = (data.size()+temp.size())/OUTLINE_2_MAX_SIZE;//计算要多少个lhp
                uint32_t beg = last;
                for(int j = 0; j < lhp_nums; j++){//处理中间的lhp
                    uint32_t lhpa;
                    create_LHP(lhpa);
                    appned_LHPI(lhpia, lhpa);//写入lhpindex
                    for(int i = 0; i < LHP_NUMS;i++){
                        if(j == 0 && i == 0){
                            append_LHP(lhpa,offset);
                            ll->H.tail2.lpas.emplace_back(offset);
                            continue;
                        }
                        create_lobpage(offset,data,beg);
                        beg += LOB_PAGE_SIZE;
                        append_LHP(lhpa,offset);
                        if(j == 0 && i < 6){
                            ll->H.tail2.lpas.emplace_back(offset);
                        }
                    }
                }
            }
            else{
                s.FetalError("插入的数据超过最大限制");
                return s;
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


///////////////////////private function///////////////////////

