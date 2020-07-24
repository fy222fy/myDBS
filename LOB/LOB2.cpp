#include "LOB_locator.h"
#include "LOB.h"

Status LOB::append_from(LOBLocator *ll,const vector<uint8_t> &data, uint32_t &beg){
    Status s;
    for(;beg < data.size(); beg += LOB_PAGE_SIZE){
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
    return s;
    LOB lob;
}