#ifndef OB_LOB_LOCATOR
#define OB_LOB_LOCATOR
#include<string>
#include<vector>
#include<../include/status.h>

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

//locator定义，拿着locator，可以对lob进行所有的操作
struct LOBLocator{
    uint32_t size;//locator的大小，B为单位
    uint8_t Locator_version;//locator的版本号，用于未来升级
    uint32_t LOBID;//LOB ID
    uint32_t LOB_version;//该LOB版本
    uint32_t type;//LOB类型
    uint32_t mode;//LOB存储方式
    uint32_t data_size;//LOB数据部分的大小
    uint32_t LOB_checksum;//校验和
    union{
        std::vector<uint8_t> data;//行内存的真实数据
        struct{
            uint32_t nums;//数量
            std::vector<uint32_t> lpas;
        }tail0;
        struct{
            uint32_t nums;//数量
            std::vector<uint32_t> lpas;
            uint32_t lhpa;//LHP的地址
        }tail1;
        struct{
            uint32_t nums;//数量
            uint32_t nums;//数量
            std::vector<uint32_t> lpas;
            uint32_t lhpia;//LHP index的地址
        }tail2;
    };
    Status append(const std::vector<uint8_t> &data);//追加数据，并修改locator结构
    Status write(uint32_t offset, const std::vector<uint8_t> &data);//覆写数据
    Status read(uint32_t amount, uint32_t offset, std::vector<uint8_t> &result);//读取数据
    Status erase(uint32_t amount, uint32_t offset);//删除部分数据
    //将locator序列化成一段数据
    static Status Serialize(std::vector<uint8_t> &result, LOBLocator **ll);
    //将一段数据反序列化成一个loblocator结构
    static Status Deserialize(const std::vector<uint8_t> &input, LOBLocator *ll);
    
};





#endif