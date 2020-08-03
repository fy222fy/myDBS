#ifndef OB_LOB_LOCATOR
#define OB_LOB_LOCATOR
#include<string>
#include<vector>
#include "../include/status.h"
#include "../VirtualFileSys/VFS.h"


//locator定义，拿着locator，可以对lob进行所有的操作
struct LOBLocator{
    uint8_t Locator_version;//locator的版本号，用于未来升级
    uint32_t size;//locator的大小，B为单位
    uint32_t LOBID;//LOB ID
    uint32_t segID;//所在的段的id
    uint32_t LOB_version;//该LOB版本
    uint32_t type;//LOB类型
    uint8_t mode;//LOB存储方式
    uint64_t data_size;//LOB数据部分的大小
    uint32_t LOB_checksum;//校验和

    uint8_t *data;//行内存的真实数据
    uint32_t lpa_nums;//保存行内lpa的数量
    std::vector<uint32_t> lpas; 
    uint64_t lhpa;//保存lhpa或者lhipa，根据类型决定
    
    static const uint32_t HEAD_SIZE = sizeof(size) + sizeof(Locator_version) +
        sizeof(LOBID) + sizeof(LOB_version) + sizeof(type) + sizeof(mode) + 
        sizeof(data_size) + sizeof(LOB_checksum);
    static const uint32_t MAX_LPA = 6;//设置行内最多有多少个LPA
    static const uint32_t INLINE_MAX_SIZE = 20;//设置行内数据最大长度
    //将locator序列化成一段数据
    static Status Serialize(std::vector<uint8_t> &result, LOBLocator **ll);
    //将一段数据反序列化成一个loblocator结构
    static Status Deserialize(const std::vector<uint8_t> &input, LOBLocator *ll);
    //指定lobid和lob类型来创建一个loblocator
    LOBLocator(uint32_t lobid, uint32_t seg_id, uint32_t t)
        :size(HEAD_SIZE),
        Locator_version(0x01),
        LOBID(lobid),
        segID(seg_id),
        type(t),
        mode(0x10),
        LOB_version(0x01),
        data_size(0),
        LOB_checksum(0){}
    ~LOBLocator();  
    void update_head(uint32_t lsize, uint8_t lmode, uint64_t dsize, uint32_t check_s){
        size = lsize;
        mode = lmode;
        data_size = dsize;
        LOB_checksum = check_s;
    }
};





#endif