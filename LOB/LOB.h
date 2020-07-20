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
struct LOBHead
{
    uint32_t size;
    static const uint32_t HEAD_SIZE = 4;//LOB页头结构的大小
};
/**
 * LOB Header Page的结构
 * size：大小
 * checksum：校验和
 * type：这个页的作用，主要是确定一阶索引还是二阶索引
*/
struct LHPHead
{
    uint32_t size;
    uint32_t checksum;
    uint8_t type;//本LHP是用来做什么的
    static const uint32_t HEAD_SIZE = 4;
};

/**
 * LOB Header Page Index的结构
 * size：大小
 * checksum：校验和
 * type：这个页的作用，主要是确定一阶索引还是二阶索引
*/
struct LHPIHead
{
    uint32_t size;
    uint32_t checksum;
    uint8_t type;//本LHP是用来做什么的
    static const uint32_t HEAD_SIZE = 4;
};

/**
 * LOB模块，负责LOB的读写等操作，主要是根据locator操作
*/
class LOB{
public:
    LOB(DataFile* df, Options *op);
    Status create(LOBLocator **llp, uint32_t seg_id);//创建一个空的lob，获取locator
    Status append(LOBLocator *ll, const std::vector<uint8_t> &data);//追加数据，并修改locator结构
    Status write(LOBLocator *ll, uint32_t offset, const std::vector<uint8_t> &data);//覆写数据
    Status read(LOBLocator *ll, uint32_t amount, uint32_t offset, std::vector<uint8_t> &result);//读取数据
    Status erase(LOBLocator *ll, uint32_t amount, uint32_t offset);//删除部分数据
    Status write(LOBLocator *ll, uint32_t offset, const std::vector<uint8_t> &data);//覆写数据
    Status read(LOBLocator *ll, uint32_t amount, uint32_t offset, std::vector<uint8_t> &result);//读取数据
    Status erase(LOBLocator *ll, uint32_t amount, uint32_t offset);//删除部分数据
    Status drop(LOBLocator *ll);//删除指向的所有lob数据
    static const uint32_t LOB_PAGE_SIZE = Segment::PAGE_FREE_SPACE - LOBHead::HEAD_SIZE;
    static const uint32_t LHP_SIZE = Segment::PAGE_FREE_SPACE - LHPHead::HEAD_SIZE;
    static const uint32_t LHP_NUMS = LHP_SIZE / 4;
    static const uint32_t LHPI_SIZE = Segment::PAGE_FREE_SPACE - LHPIHead::HEAD_SIZE;
    static const uint32_t LHPI_NUMS = LHPI_SIZE / 4;
    static const uint32_t OUTLINE_1_MAX_SIZE = LOBLocator::MAX_LPA * LOB_PAGE_SIZE;//行外存1模式最大数据量
    static const uint32_t OUTLINE_2_MAX_SIZE = LHP_NUMS * LOB_PAGE_SIZE;
    static const uint32_t OUTLINE_3_MAX_SIZE = LHPI_NUMS * LHP_NUMS * LOB_PAGE_SIZE;
    
    /**
     * 
    */
private:
    uint32_t checksum(LOBLocator *ll);//校验和计算
    Options *op;
    DataFile *df;
    Status create_lobpage(uint32_t &offset,const vector<uint8_t> &data,uint32_t beg);
    Status write_lobpage(uint32_t offset);
    Status read_lobpage(uint32_t offset);
    Status create_LHP(uint32_t &lhpa);
    Status append_LHP(uint32_t lhpa, uint32_t offset);
    Status write_LHP();
    Status read_LHP();
    Status create_LHPI(uint32_t &lhpia);
    Status appned_LHPI(uint32_t &lhpia, uint32_t lhpa);
    Status write_LHPI();
    Status read_LHPI();
};

#endif //fdsa