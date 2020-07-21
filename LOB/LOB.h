#ifndef OB_LOB_INTERFACE
#define OB_LOB_INTERFACE
#include<string>
#include<vector>
#include "../include/status.h"
#include "LOB_locator.h"

/**
 * LOB Page的结构
 * size：大小
 * checksum：校验和
 * type：这个页的作用，主要是确定一阶索引还是二阶索引
*/
struct LOBP
{
    LOBP():size(0),checksum(0),type(0x00){}
    ~LOBP(){}
    uint32_t size;
    uint32_t checksum;
    uint8_t type;
    vector<uint8_t> *data;//实际数据
    static const uint32_t HEAD_SIZE = 4;//LOB页头结构的大小
    Status append(const vector<uint8_t> *data, uint32_t beg, uint32_t len){
        ;
    }
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
    static const uint32_t HEAD_SIZE = 4;
    Status append(uint32_t lpa){
        M.emplace_back(lpa);
        nums++;
    }//加入一个lpa
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
    static const uint32_t HEAD_SIZE = 4;
    Status append(uint32_t lpia){
        M.emplace_back(lpia);
        nums++;
    }//加入一个lpa
};

/**
 * LOB模块，负责LOB的读写等操作，主要是根据locator操作
*/
class LOB{
public:
    LOB();
    Status create(LOBLocator **llp);//创建一个空的lob，获取locator
    Status append(LOBLocator *ll, const std::vector<uint8_t> &data);//追加数据，并修改locator结构
    Status write(LOBLocator *ll, uint32_t offset, const std::vector<uint8_t> &data);//覆写数据
    Status read(LOBLocator *ll, uint32_t amount, uint32_t offset, std::vector<uint8_t> &result);//读取数据
    Status erase(LOBLocator *ll, uint32_t amount, uint32_t offset);//删除部分数据
    Status drop(LOBLocator *ll);//删除指向的所有lob数据
    static const uint32_t LOB_PAGE_SIZE = VFS::PAGE_FREE_SPACE - LOBP::HEAD_SIZE;
    static const uint32_t LHP_SIZE = VFS::PAGE_FREE_SPACE - LOBP::HEAD_SIZE;
    static const uint32_t LHP_NUMS = LHP_SIZE / 4;
    static const uint32_t LHPI_SIZE = VFS::PAGE_FREE_SPACE - LOBP::HEAD_SIZE;
    static const uint32_t LHPI_NUMS = LHPI_SIZE / 4;
    static const uint32_t OUTLINE_1_MAX_SIZE = LOBLocator::MAX_LPA * LOB_PAGE_SIZE;//行外存1模式最大数据量
    static const uint32_t OUTLINE_2_MAX_SIZE = LHP_NUMS * LOB_PAGE_SIZE;
    static const uint32_t OUTLINE_3_MAX_SIZE = LHPI_NUMS * LHP_NUMS * LOB_PAGE_SIZE;
private:
    Options *op;
    uint32_t checksum(LOBLocator *ll);//校验和计算
    Status create_lobpage(uint32_t &offset, const vector<uint8_t> &data, uint32_t beg, uint32_t len);
    //Status append_lobpage(uint32_t &offset, LOBP *lobp);
    Status write_lobpage(uint32_t offset, LOBP *lobp);
    Status read_lobpage(uint32_t offset, LOBP **lobp);
    Status free_lobpage(uint32_t offset);
    Status create_LHP(LHP **lhp);
    Status append_LHP(uint32_t &offset, LHP *lhp);
    Status write_LHP(uint32_t offset, LHP *lhp);
    Status read_LHP(uint32_t offset, LHP **lhp);
    Status free_LHP();
    Status create_LHIP(LHIP **lhip);
    Status appned_LHIP(uint32_t &offset, LHIP *lhip);
    Status write_LHIP(uint32_t offset, LHIP *lhip);
    Status read_LHIP(uint32_t offset, LHIP **lhip);
    Status free_LHIP();
    uint32_t new_lob_id();
};

#endif //fdsa