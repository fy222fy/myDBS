#ifndef OB_LOB_LOCATOR
#define OB_LOB_LOCATOR
#include<string>
#include<cstring>
#include<vector>
#include "../include/status.h"
#include "../VirtualFileSys/VFS.h"

//locator定义，拿着locator，可以对lob进行所有的操作
struct LOBLocator{
public:
    static const uint32_t MAX_LPA = 6;//设置行内最多有多少个LPA
    static const uint32_t INLINE_MAX_SIZE = 100;//设置行内数据最大长度
private:
    uint8_t Locator_version;//locator的版本号，用于未来升级
    uint32_t size;//locator的大小，B为单位
    uint32_t LOBID;//LOB ID
    uint32_t segID;//所在的段的id
    uint32_t LOB_version;//该LOB版本
    uint32_t type;//LOB类型
    uint8_t mode;//LOB存储方式
    uint64_t data_size;//LOB数据部分的大小
    uint64_t inrow_data_size;//行内存的lpa，指向的datasize
    uint32_t LOB_checksum;//校验和

    uint8_t *data;//行内存的真实数据
    uint64_t lpas[MAX_LPA]; //保存所有的lpa以及他们对应的长度
    uint32_t lpa_nums;//保存行内lpa数量
    uint64_t lpas_sizes[MAX_LPA];
    uint64_t lhpa;//保存lhpa或者lhipa，根据类型决定
public:
    static const uint32_t HEAD_SIZE = sizeof(size) + sizeof(Locator_version) +
        sizeof(LOBID) + sizeof(LOB_version) + sizeof(type) + sizeof(mode) + 
        sizeof(data_size) + sizeof(LOB_checksum);
    //将locator序列化成一段数据
    Status Serialize(uint8_t *result);
    //将一段数据反序列化成一个loblocator结构
    Status Deserialize(const uint8_t *input);
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
    uint32_t get_head_size()const{return size;}
    void init(uint32_t lobid, uint32_t seg_id, uint32_t t){
        LOBID = lobid;
        segID = seg_id;
        type = t;
    }
    ~LOBLocator();  
    void update_head(uint8_t new_mode, uint64_t new_data_size){    //还需要写一些东西
        mode = new_mode;
        data_size = new_data_size;
    }
    Status append_lpas(uint64_t addr, uint32_t d_size){
        Status s;
        if(lpa_nums > MAX_LPA) s.FetalError("行内数量达到上限了，不要再加了");
        lpas[lpa_nums]= addr;
        lpas_sizes[lpa_nums] = d_size;
        lpa_nums++;
        inrow_data_size += d_size;
        return s;
    }
    vector<pair<uint64_t, uint64_t>> get_all_lpas(){
        vector<pair<uint64_t, uint64_t>> result;
        for(int i = 0; i < lpa_nums; i++){
            result.emplace_back(make_pair(lpas[i],lpas_sizes[i]));
        }
        return result;
    }
    uint32_t get_lpa_nums() const{return lpa_nums;}
    uint64_t get_LHPA()const{return lhpa;}
    void set_LHPA(uint64_t _lhpa){lhpa = _lhpa;mode=0x12;}
    uint64_t get_LHIPA() const{return lhpa;}
    void set_LHIPA(uint64_t _lhipa) {lhpa = _lhipa;mode=0x13;}
    void set_data_size(uint64_t d_size) {data_size = d_size;}
    uint8_t get_mode() const {return mode;}
    uint8_t *get_data() {return data;}
    void set_data(uint8_t *new_data, uint64_t len){
        free_data();
        data = new uint8_t[len];
        memcpy(data,new_data,len);
    }
    void free_data() {delete[] data;}
    uint64_t get_data_size() const {return data_size;}
    uint64_t get_inrow_data_size() const {return inrow_data_size;}
    uint32_t get_seg_id() const {return segID;}
    bool is_inline_full() const {return lpa_nums >= MAX_LPA;}
    void update_version(){LOB_version++;}
    uint32_t get_version() const {return LOB_version;}
    void clear_all_lpas(){
        lpa_nums = 0;
        inrow_data_size = 0;
    }
};





#endif