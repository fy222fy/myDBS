#ifndef OB_LOB_INTERFACE
#define OB_LOB_INTERFACE
#include<string>
#include<vector>
#include "../include/status.h"
#include "LOB_locator.h"

/**
 * LOB Header Page的结构
 * size：大小
 * checksum：校验和
 * type：这个页的作用，主要是确定一阶索引还是二阶索引
*/
struct LHP
{
    uint32_t size;
    uint32_t checksum;
    uint8_t type;//本LHP是用来做什么的
};

/**
 * LOB Header Page Index的结构
 * size：大小
 * checksum：校验和
 * type：这个页的作用，主要是确定一阶索引还是二阶索引
*/
struct LHPIndex
{
    uint32_t size;
    uint32_t checksum;
    uint8_t type;//本LHP是用来做什么的
};

/**
 * LOB模块，负责LOB的读写等操作，主要是根据locator操作
*/
class LOB{
public:
    /**
     * 创建一个空的lob，获取locator
     * write_data
     * read_data
     * write_LHP
     * read_LHP
     * write_LHPIndex
     * 
    */
    Status append(LOBLocator *ll);//追加数据，并修改locator结构
    Status write(LOBLocator *ll, uint32_t offset, const std::vector<uint8_t> &data);//覆写数据
    Status read(LOBLocator *ll, uint32_t amount, uint32_t offset, std::vector<uint8_t> &result);//读取数据
    Status erase(LOBLocator *ll, uint32_t amount, uint32_t offset);//删除部分数据
    /**
     * 
    */
private:
    
};

#endif //fdsa