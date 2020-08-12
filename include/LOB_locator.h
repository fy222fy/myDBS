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
    static const uint32_t HEAD_BASIC_SIZE =  sizeof(Locator_version) + sizeof(size) +
        sizeof(LOBID) + sizeof(segID) + sizeof(LOB_version) + sizeof(type) + sizeof(mode) + 
        sizeof(data_size) + sizeof(inrow_data_size) +sizeof(LOB_checksum);
    //将locator序列化成一段数据
    Status Serialize(uint8_t *result){
        Status s;
        Convert convert;
        uint8_t *temp;
        int it = 0;
        result[it++] = Locator_version;
        temp = convert.int32_to_int8(size);
        for(int i = 0; i < 4; i++) result[it++] = temp[i];
        temp = convert.int32_to_int8(LOBID);
        for(int i = 0; i < 4; i++) result[it++] = temp[i];
        temp = convert.int32_to_int8(segID);
        for(int i = 0; i < 4; i++) result[it++] = temp[i];
        temp = convert.int32_to_int8(LOB_version);
        for(int i = 0; i < 4; i++) result[it++] = temp[i];
        temp = convert.int32_to_int8(type);
        for(int i = 0; i < 4; i++) result[it++] = temp[i];
        result[it++] = mode;
        temp = convert.int64_to_int8(data_size);
        for(int i = 0; i < 8; i++) result[it++] = temp[i];
        temp = convert.int64_to_int8(inrow_data_size);
        for(int i = 0; i < 8; i++) result[it++] = temp[i];
        temp = convert.int32_to_int8(LOB_checksum);
        for(int i = 0; i < 4; i++) result[it++] = temp[i];
        if(mode == 0x10){//表示行内存储
            memcpy(result+it,data,data_size);
        }
        else{
            temp = convert.int32_to_int8(lpa_nums);
            for(int i = 0; i < 4; i++) result[it++] = temp[i];
            for(int j = 0; j < lpa_nums;j++){
                temp = convert.int64_to_int8(lpas[j]);
                for(int i = 0; i < 8; i++) result[it++] = temp[i];
                temp = convert.int64_to_int8(lpas_sizes[j]);
                for(int i = 0; i < 8; i++) result[it++] = temp[i];
            }
            if(mode != 0x11){
                temp = convert.int64_to_int8(lhpa);
                for(int i = 0; i < 8; i++) result[it++] = temp[i];
            }
        }
        return s;
    }
    //将一段数据反序列化成一个loblocator结构
    Status Deserialize(const uint8_t *input){
        Status s;
        Convert convert;
        int it = 0;
        Locator_version = input[it++];
        size = convert.int8_to_int32(input,it); it+=4;
        LOBID = convert.int8_to_int32(input,it); it+=4;
        segID = convert.int8_to_int32(input,it); it+=4;
        LOB_version = convert.int8_to_int32(input,it); it+=4;
        type = convert.int8_to_int32(input,it); it+=4;
        mode = input[it++];
        data_size = convert.int8_to_int64(input,it); it+=8;
        inrow_data_size = convert.int8_to_int64(input,it); it+=8;
        LOB_checksum = convert.int8_to_int32(input,it); it+=4;
        if(mode == 0x10){
            uint8_t *temp = const_cast<uint8_t *>(input);
            set_data(temp + it, data_size);
        }
        else{
            lpa_nums = convert.int8_to_int32(input,it); it+=4;
            for(int j = 0; j < lpa_nums; j++){
                lpas[j] = convert.int8_to_int64(input,it); it+=8;
                lpas_sizes[j] = convert.int8_to_int64(input,it); it+=8;
            }
            if(mode != 0x11){
                lhpa  = convert.int8_to_int64(input,it); it+=8;
            }
        }
        return s;
    }
    //指定lobid和lob类型来创建一个loblocator
    LOBLocator()
        :size(HEAD_BASIC_SIZE),
        Locator_version(0x01),
        LOBID(0),
        segID(0),
        type(0),
        mode(0x10),
        LOB_version(0x01),
        data_size(0),
        LOB_checksum(0),
        data(nullptr){}
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
        if(mode == 0x10) size= HEAD_BASIC_SIZE + data_size;
        else if(mode == 0x11){
            size = HEAD_BASIC_SIZE + lpa_nums * 16 + 4;//还要加一个lpanums
        }
        else{
            size = HEAD_BASIC_SIZE + lpa_nums *16 + 4 + 8;//还要再加一个lpanums和一个lhpa
        }
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
        if(len > 0){
            data = new uint8_t[len];
            memcpy(data,new_data,len);
        }
    }
    void free_data() {
        if(data != nullptr){
            delete[] data;
            data == nullptr;
        }
    }
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