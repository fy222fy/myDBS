#ifndef OB_LOB_LOCATOR
#define OB_LOB_LOCATOR
#include<string>
#include<vector>
#include<../include/status.h>
#include "../VirtualFileSys/VFS.h"


//locator定义，拿着locator，可以对lob进行所有的操作
struct LOBLocator{
    uint32_t size;//locator的大小，B为单位
    uint8_t Locator_version;//locator的版本号，用于未来升级
    uint32_t LOBID;//LOB ID
    uint32_t LOB_version;//该LOB版本
    uint32_t type;//LOB类型
    uint8_t mode;//LOB存储方式
    uint32_t data_size;//LOB数据部分的大小
    uint32_t LOB_checksum;//校验和
    union Handle{
        Handle(){}
        ~Handle(){}
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
    }H;
    static const uint32_t HEAD_SIZE = sizeof(size) + sizeof(Locator_version) +
        sizeof(LOBID) + sizeof(LOB_version) + sizeof(type) + sizeof(mode) + 
        sizeof(data_size) + sizeof(LOB_checksum);
    static const uint32_t MAX_LPA = 6;//设置行内最多有多少个LPA
    static const uint32_t INLINE_MAX_SIZE = 20;//设置行内数据最大长度
    //将locator序列化成一段数据
    static Status Serialize(std::vector<uint8_t> &result, LOBLocator **ll);
    //将一段数据反序列化成一个loblocator结构
    static Status Deserialize(const std::vector<uint8_t> &input, LOBLocator *ll);
    LOBLocator(uint32_t lobid, uint32_t t)
        :size(HEAD_SIZE),
        Locator_version(0x01),
        LOBID(lobid),
        type(t),
        mode(0x01),
        LOB_version(0x01),
        data_size(0),
        LOB_checksum(0){}
    ~LOBLocator();  
};





#endif