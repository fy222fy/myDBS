#ifndef OB_LOB_INTERFACE
#define OB_LOB_INTERFACE
#include<string>
#include<vector>
#include "../include/status.h"
#include "LOB_locator.h"
#include "../Util/Util.h"
struct LOBH;
struct LHP;
struct LHIP;
/**
 * LOB模块，负责LOB的读写等操作，主要是根据locator操作
*/

/**
 * LOB Page的结构
 * size：大小
 * checksum：校验和
 * type：这个页的作用，主要是确定一阶索引还是二阶索引
*/
struct LOBH
{
    LOBH(uint32_t s,uint32_t cks):size(s),checksum(cks),type(0x00){}
    ~LOBH(){}
    uint32_t size;
    uint32_t checksum;
    uint8_t type;
    static const uint32_t HEAD_SIZE = 8;//LOB页头结构的大小
};
/**
 * LOB Header Page的结构
 * size：大小
 * checksum：校验和
 * type：这个页的作用，主要是确定一阶索引还是二阶索引
*/
struct LHP
{
    LHP():nums(0),checksum(0),type(0x01){}
    ~LHP(){vector<uint32_t>().swap(M);}
    uint32_t nums;
    uint32_t checksum;
    uint8_t type;//本LHP是用来做什么的
    vector<uint32_t> M;
    static const uint32_t HEAD_SIZE = 8;
    static const uint32_t MAX_LPA = (VFS::PAGE_FREE_SPACE - HEAD_SIZE) / 4;
    Status append(uint32_t lpa){
        M.emplace_back(lpa);
        nums++;
    }//加入一个lpa
    Status Serialize(vector<uint8_t> &result){
        uint8_t *temp = int32_to_int8(nums);
        for(int i = 0; i < 4;i++) result.emplace_back(temp[i]);
        temp = int32_to_int8(nums);
        for(int i = 0; i < 4;i++) result.emplace_back(temp[i]);
        for(int i = 0; i < nums; i++){
            temp = int32_to_int8(nums);
            for(int i = 0; i < 4;i++) result.emplace_back(temp[i]);
        }
    }
    Status Deserialize(const vector<uint8_t> &input){
        int it = 0;
        nums = int8_to_int32(input,it);
        it+=4;
        checksum = int8_to_int32(input,it);
        it+=4;
        for(;it<input.size();it=it+4){
            append(int8_to_int32(input,it));
        }
    }
};

/**
 * LOB Header Page Index的结构
 * size：大小
 * checksum：校验和
 * type：这个页的作用，主要是确定一阶索引还是二阶索引
*/
struct LHIP
{
    LHIP():nums(0),checksum(0),type(0x02){}
    ~LHIP(){vector<uint32_t>().swap(M);}
    uint32_t nums;
    uint32_t checksum;
    uint8_t type;//类型
    vector<uint32_t> M;
    static const uint32_t HEAD_SIZE = 8;
    Status append(uint32_t lhpa){
        M.emplace_back(lhpa);
        nums++;
    }//加入一个lpa
    uint32_t read_last_lhpa(){
        return *(M.end() - 1);
    }
    Status Serialize(vector<uint8_t> &result){
        uint8_t *temp = int32_to_int8(nums);
        for(int i = 0; i < 4;i++) result.emplace_back(temp[i]);
        temp = int32_to_int8(nums);
        for(int i = 0; i < 4;i++) result.emplace_back(temp[i]);
        for(int i = 0; i < nums; i++){
            temp = int32_to_int8(nums);
            for(int i = 0; i < 4;i++) result.emplace_back(temp[i]);
        }
    }
    Status Deserialize(const vector<uint8_t> &input){
        int it = 0;
        nums = int8_to_int32(input,it);
        it+=4;
        checksum = int8_to_int32(input,it);
        it+=4;
        for(;it<input.size();it=it+4){
            append(int8_to_int32(input,it));
        }
    }
};
class LOB{
public:
    LOB();
    Status create_locator(LOBLocator **llp, uint32_t seg_id);//创建一个空的lob，获取locator
    Status create_lobseg(uint32_t &seg_id);//创建一个lob段，获取段的id
    Status append(LOBLocator *ll, const std::vector<uint8_t> &data);//追加数据，并修改locator结构
    Status write(LOBLocator *ll, uint32_t offset, const std::vector<uint8_t> &data);//覆写数据
    Status read(LOBLocator *ll, uint32_t amount, uint32_t offset, std::vector<uint8_t> &result);//读取数据
    Status erase(LOBLocator *ll, uint32_t amount, uint32_t offset);//删除部分数据
    Status drop(LOBLocator *ll);//删除指向的所有lob数据
    static const uint32_t LOB_PAGE_SIZE = VFS::PAGE_FREE_SPACE - LOBH::HEAD_SIZE;
    static const uint32_t LHP_SIZE = VFS::PAGE_FREE_SPACE - LOBH::HEAD_SIZE;
    static const uint32_t LHP_NUMS = LHP_SIZE / 4;
    static const uint32_t LHPI_SIZE = VFS::PAGE_FREE_SPACE - LOBH::HEAD_SIZE;
    static const uint32_t LHPI_NUMS = LHPI_SIZE / 4;
    static const uint32_t OUTLINE_1_MAX_SIZE = LOBLocator::MAX_LPA * LOB_PAGE_SIZE;//行外存1模式最大数据量
    static const uint32_t OUTLINE_2_MAX_SIZE = LHP_NUMS * LOB_PAGE_SIZE;
    static const uint32_t OUTLINE_3_MAX_SIZE = LHPI_NUMS * LHP_NUMS * LOB_PAGE_SIZE;
private:
    Options *op;
    uint32_t checksum(LOBLocator *ll);//校验和计算
    //在指定段上创建一个lob页，然后写入指定长度的数据 
    Status create_lobpage(uint32_t segid, uint32_t &offset, const vector<uint8_t> &data, uint32_t beg, uint32_t len);
    //Status append_lobpage(uint32_t &offset, LOBP *lobp);
    Status write_lobpage(uint32_t segid, uint32_t offset, const vector<uint8_t> &data, uint32_t beg, uint32_t len);
    Status read_lobpage(uint32_t segid, uint32_t offset, vector<uint8_t> &output);
    Status free_lobpage(uint32_t offset);
    Status create_LHP(LHP **lhp);
    //Status append_LHP(uint32_t &offset, LHP *lhp);
    Status write_LHP(uint32_t segid, uint32_t offset, LHP *lhp);
    Status read_LHP(uint32_t segid, uint32_t offset, LHP **lhp);
    Status free_LHP();
    Status create_LHIP(LHIP **lhip);
    //Status append_LHIP(uint32_t &offset, LHIP *lhip);
    Status write_LHIP(uint32_t segid, uint32_t offset, LHIP *lhip);
    Status read_LHIP(uint32_t segid, uint32_t offset, LHIP **lhip);
    Status free_LHIP();
    uint32_t new_lob_id();
};

#endif //fdsa