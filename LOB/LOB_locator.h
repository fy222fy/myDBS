#ifndef OB_LOB_LOCATOR
#define OB_LOB_LOCATOR
#include<string>
#include<vector>
#include "../include/status.h"
#include "../VirtualFileSys/VFS.h"

//locator定义，拿着locator，可以对lob进行所有的操作
struct LOBLocator{
public:
    static const uint32_t MAX_LPA = 6;//设置行内最多有多少个LPA
    static const uint32_t INLINE_MAX_SIZE = 20;//设置行内数据最大长度
private:
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
    uint64_t lpas[MAX_LPA]; //保存所有的lpa以及他们对应的长度
    uint32_t lpa_nums;//保存行内lpa数量
    uint32_t lpas_sizes[MAX_LPA];
    uint64_t lhpa;//保存lhpa或者lhipa，根据类型决定
public:
    static const uint32_t HEAD_SIZE = sizeof(size) + sizeof(Locator_version) +
        sizeof(LOBID) + sizeof(LOB_version) + sizeof(type) + sizeof(mode) + 
        sizeof(data_size) + sizeof(LOB_checksum);
    //将locator序列化成一段数据
    static Status Serialize(uint8_t *result, LOBLocator **ll);
    //将一段数据反序列化成一个loblocator结构
    static Status Deserialize(const uint8_t *input, LOBLocator *ll);
    //指定lobid和lob类型来创建一个loblocator
    LOBLocator()
        :size(HEAD_SIZE),
        Locator_version(0x01),
        LOBID(0),
        segID(0),
        type(0),
        mode(0x10),
        LOB_version(0x01),
        data_size(0),
        LOB_checksum(0){}

    void init(uint32_t lobid, uint32_t seg_id, uint32_t t){
        LOBID = lobid;
        segID = seg_id;
        type = t;
    }
    ~LOBLocator();  
    void update_head(){    //还需要写一些东西
        ;
    }
    Status append_lpas(uint64_t addr, uint32_t data_size){
        Status s;
        if(lpa_nums > MAX_LPA) s.FetalError("行内数量达到上限了，不要再加了");
        lpas[lpa_nums]= addr;
        lpas_sizes[lpa_nums] = data_size;
        lpa_nums++;
        return s;
    }
    vector<pair<uint64_t, uint64_t>> get_all_lpas(){
        vector<pair<uint64_t, uint64_t>> result;
        for(int i = 0; i < lpa_nums; i++){
            result.emplace_back(make_pair(lpas[i],lpas_sizes[i]));
        }
        return result;
    }
    uint64_t get_LHPA()const{return lhpa;}
    void set_LHPA(uint64_t _lhpa){lhpa = _lhpa;mode=0x12;}
    uint64_t get_LHIPA() const{return lhpa;}
    void set_LHIPA(uint64_t _lhipa) {lhpa = _lhipa;mode=0x13;}
    void set_mode(uint8_t new_mode){mode = new_mode;}
    uint8_t get_mode() const {return mode;}
    uint8_t *get_data() {return data;}
    void set_data(uint8_t *new_data, uint64_t len){
        free_data();
        data = new_data;
        data_size = len;
    }
    void free_data(){delete[] data;}
    uint64_t get_data_size() const {return data_size;}
    uint32_t get_seg_id() const {return segID;}
    bool is_inline_full() const {return lpa_nums >= MAX_LPA;}
};





#endif