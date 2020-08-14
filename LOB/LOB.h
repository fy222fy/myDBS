#ifndef OB_LOB_INTERFACE
#define OB_LOB_INTERFACE
#include<string>
#include<cstring>
#include<vector>
#include "../include/status.h"
#include "../include/LOB_locator.h"
#include "../Util/Util.h"

struct LHP;
struct LHIP;


/**
 * LOB Page的结构
 * size：大小
 * checksum：校验和
 * type：这个页的作用，主要是确定一阶索引还是二阶索引
*/
struct LOBP
{
    LOBP(const uint8_t *dta, uint64_t l):data(dta),data_size(l),checksum(1280262656),type(0x00){}
    LOBP():data_size(0),checksum(0),type(0x00),version(0x00){}
    
    ~LOBP(){}
    uint64_t data_size;
    uint32_t checksum;
    uint32_t version;//版本号
    uint8_t type;
    const uint8_t *data;
    static const uint32_t HEAD_SIZE = 12;//LOB页头结构的大小
    void Serialize(uint8_t *output){
        Convert convert;
        uint32_t it = 0;
        uint8_t *temp = convert.int64_to_int8(data_size);
        for(int i = 0; i < 8;i++) output[it++] = temp[i];
        temp = convert.int32_to_int8(checksum);
        for(int i = 0; i < 4;i++) output[it++] = temp[i];
        memcpy(output + HEAD_SIZE,data,data_size);
    }
    void Deserialize(const uint8_t *input){
        Convert convert;
        uint32_t it = 0 ;
        data_size = convert.int8_to_int64(input,it);
        it+=8;
        checksum = convert.int8_to_int32(input,it);
        it+=4;
        uint8_t *tmp = new uint8_t[data_size];
        memcpy(tmp,input+HEAD_SIZE,data_size);
        data = tmp;
    }
private:
    LOBP(const uint8_t *dta):data(dta){}
};
/**
 * LOB Header Page的结构
 * size：大小
 * checksum：校验和
 * type：这个页的作用，主要是确定一阶索引还是二阶索引
*/
struct LHP
{
    LHP():data_size(0),nums(0),checksum(1279807488),type(0x01),version(0x00){}
    ~LHP(){vector<pair<uint64_t,uint64_t>>().swap(M);}
    uint32_t nums;//LHP中lpa的个数
    uint32_t checksum;
    uint8_t type;//本LHP是用来做什么的
    uint64_t data_size;
    uint32_t version;//版本号
    vector<pair<uint64_t,uint64_t>> M;
    static const uint32_t HEAD_SIZE = 8;
    static const uint64_t LHP_SIZE = VFS::PAGE_FREE_SPACE;
    static const uint32_t MAX_LPA = (VFS::PAGE_FREE_SPACE - HEAD_SIZE) / 16;
    void append(uint64_t lpa,uint64_t size){
        M.emplace_back(make_pair(lpa,size));
        data_size += size;
        nums++;
    }//加入一个lpa

    void Serialize(uint8_t *result){
        Convert convert;
        uint8_t *temp = convert.int32_to_int8(nums);
        int it = 0;
        for(int i = 0; i < 4;i++) result[it++] = temp[i];
        temp = convert.int32_to_int8(checksum);
        for(int i = 0; i < 4;i++) result[it++] = temp[i];
        temp = convert.int64_to_int8(data_size);
        for(int i = 0; i < 8;i++) result[it++] = temp[i];
        for(int j = 0; j < nums; j++){
            temp = convert.int64_to_int8(M[j].first); //序列化lpa
            for(int i = 0; i < 8;i++) result[it++] = temp[i];
            temp = convert.int64_to_int8(M[j].second);//序列化每个lpa对应的size
            for(int i = 0; i < 8;i++) result[it++] = temp[i];
        }
    }
    void Deserialize(const uint8_t *input){
        Convert convert;
        int it = 0;
        nums = convert.int8_to_int32(input,it);
        it+=4;
        checksum = convert.int8_to_int32(input,it);
        it+=4;
        data_size = convert.int8_to_int64(input,it);
        it+=8;
        for(int i = 0;i < nums;i++){
            uint64_t addr = convert.int8_to_int64(input,it);
            it+=8;
            uint64_t size = convert.int8_to_int64(input,it);
            it+=8;
            M.emplace_back(make_pair(addr,size));
        }
    }
    bool is_full()const{return nums >= MAX_LPA;}
    bool is_empty()const{return nums == 0;}
    uint64_t get_data_size()const{return data_size;}
    vector<pair<uint64_t,uint64_t>> get_all_lpas(){return M;}
};

/**
 * LOB Header Page Index的结构
 * size：大小
 * checksum：校验和
 * type：这个页的作用，主要是确定一阶索引还是二阶索引
*/
struct LHIP
{
    LHIP():nums(0),checksum(1279805776),type(0x02),version(0x00),data_size(0){}
    ~LHIP(){vector<pair<uint64_t,uint64_t>>().swap(M);}
    uint32_t nums;
    uint32_t checksum;
    uint64_t data_size;
    uint8_t type;//类型
    uint32_t version;//版本号
    vector<pair<uint64_t,uint64_t>> M;
    static const uint32_t HEAD_SIZE = 8;
    static const uint64_t LHIP_SIZE = VFS::PAGE_FREE_SPACE;
    void append(uint64_t lhpa, uint64_t size){
        M.emplace_back(make_pair(lhpa,size));
        nums++;
        data_size += size;
    }//加入一个lpa
    uint64_t read_last_lhpa(){ return M[nums-1].first;}
    void remove_last_lhpa(){data_size -= M[nums-1].second; M.pop_back(); nums-=1;}
    void Serialize(uint8_t *result){
        Convert convert;
        int it = 0;
        uint8_t *temp = convert.int32_to_int8(nums);
        for(int i = 0; i < 4;i++) result[it++] = temp[i];
        temp = convert.int32_to_int8(checksum);
        for(int i = 0; i < 4;i++) result[it++] = temp[i];
        temp = convert.int64_to_int8(data_size);
        for(int i = 0; i < 8;i++) result[it++] = temp[i];
        for(int j = 0; j < nums; j++){
            temp = convert.int64_to_int8(M[j].first); //序列化lhpa
            for(int i = 0; i < 8;i++) result[it++] = temp[i];
            temp = convert.int64_to_int8(M[j].second);//序列化每个lhpa对应的size
            for(int i = 0; i < 8;i++) result[it++] = temp[i];
        }
    }
    void Deserialize(const uint8_t *input){
        Convert convert;
        int it = 0;
        nums = convert.int8_to_int32(input,it);
        it+=4;
        checksum = convert.int8_to_int32(input,it);
        it+=4;
        data_size = convert.int8_to_int64(input,it);
        it+=8;
        for(int i = 0;i < nums; i++){
            uint64_t addr = convert.int8_to_int64(input,it);
            it+=8;
            uint64_t size = convert.int8_to_int64(input,it);
            it+=8;
            M.emplace_back(make_pair(addr,size));
        }
    }
    vector<pair<uint64_t,uint64_t>> get_all_lhps(){return M;}
};
///LOB模块，负责LOB的读写等操作，主要是根据locator操作
///
///
class LOBimpl{
public:
    LOBimpl(LOBLocator *ll);
    /// 追加数据
    /// \param ll lOBLocator
    /// \param data 要追加的数据
    /// \param len 需要追加的长度
    Status append(const uint8_t *data, uint64_t len);//追加数据，并修改locator结构
    /// 插入数据
    /// \param ll lOBLocator
    /// \param data_off 从什么地方开始写入
    /// \param data 要写入的数据
    /// \param len 需要写入的长度
    Status insert(uint64_t data_off, const uint8_t *data, uint64_t len);//覆写数据
    /// 读取数据
    /// \param ll lOBLocator
    /// \param data_off 从什么地方开始读取
    /// \param result 传出读取的数据
    /// \param amount 要读取的数据长度
    Status read(uint64_t data_off, uint8_t *result, uint64_t amount);//读取数据
    /// 擦除数据
    /// \param ll lOBLocator
    /// \param data_off 从什么地方开始擦除
    /// \param amount 要擦除的数据长度
    Status erase(uint64_t data_off, uint64_t amount);//删除部分数据    
    Status finish();//完成操作，统一写入。
    void add_lpa(uint64_t addr,uint64_t size){
        all_lpa.emplace_back(make_pair(addr,size));
        data_size += size;
    }
    void insert_lpa(uint64_t addr, uint64_t size, int index){
        all_lpa.insert(all_lpa.begin()+index,make_pair(addr,size));
        data_size += size;
    }
    void remove_lpa(int index){
        data_size -= all_lpa[index].second;
        all_lpa.erase(all_lpa.begin()+index);
    }
    static const uint64_t LOB_PAGE_SIZE = VFS::PAGE_FREE_SPACE - LOBP::HEAD_SIZE;
    static const uint64_t LHP_SIZE = VFS::PAGE_FREE_SPACE - LOBP::HEAD_SIZE;
    static const uint64_t LHP_NUMS = LHP_SIZE / 16;
    static const uint64_t LHPI_SIZE = VFS::PAGE_FREE_SPACE - LOBP::HEAD_SIZE;
    static const uint64_t LHPI_NUMS = LHPI_SIZE / 16;
    static const uint64_t OUTLINE_1_MAX_SIZE = LOBLocator::MAX_LPA * LOB_PAGE_SIZE;//行外存1模式最大数据量
    static const uint64_t OUTLINE_2_MAX_SIZE = LHP_NUMS * LOB_PAGE_SIZE;
    static const uint64_t OUTLINE_3_MAX_SIZE = LHPI_NUMS * LHP_NUMS * LOB_PAGE_SIZE;
private:
    Options *op;
    uint32_t checksum();//校验和计算
    //在指定段上创建一个lob页，然后写入指定长度的数据 
    Status create_lobpage(uint64_t &offset, const uint8_t *data, uint64_t len);
    //Status append_lobpage(uint32_t &offset, LOBP *lobp);
    Status write_lobpage( uint64_t offset, const uint8_t *data, uint64_t len);
    Status read_lobpage(uint64_t offset, uint8_t *output, uint64_t len);
    Status free_lobpage(uint32_t offset);
    Status append_LHP(uint64_t &offset,LHP lhp);
    Status write_LHP(uint64_t offsetp,LHP lhp);
    Status read_LHP(uint64_t offset, LHP &lhp);
    Status free_LHP(LHP &lhp);
    Status append_LHIP(uint64_t &offset, LHIP lhip);
    Status write_LHIP(uint64_t offset, LHIP lhip);
    Status read_LHIP(uint64_t offset, LHIP &lhip);
    Status free_LHIP( LHIP &lhip);
    //在out-1结构中插入数据，如果能全部插入就全部插入，否则插满返回即可
    Status insert_out1(const uint8_t *data);
    LOBLocator *ll;//一个lobimpl负责一个locator的操作
    uint32_t segid;//locator对应的段id
    uint32_t cur_version;//表示这次的实施的版本号
    uint8_t mode;
    vector<pair<uint64_t,uint64_t>> all_lpa;//保存了所有的lpa
    bool if_inrow;
    uint8_t *inrow_data;//不断用到的data
    uint64_t data_size;//数据的大小
    void set_inrow_data(const uint8_t *new_data, uint64_t len){
        free_inrow_data();
        if(len != 0){
            inrow_data = new uint8_t[len];
            memcpy(inrow_data,new_data,len);
            data_size = len;
            if_inrow = true;
        }
    }
    void free_inrow_data(){
        if(inrow_data != nullptr){
            delete[] inrow_data; 
            inrow_data = nullptr;
        } 
        data_size = 0;
        if_inrow = false;
    }
    

};

//LOB接口
class LOB{
public:
    LOB(){}
    //创建一个空的LOcator
    Status create_locator(LOBLocator *llp, uint32_t seg_id);
    //创建一个空的段
    Status create_lobseg(uint32_t &seg_id);
    /// 比较两个LOB中的一段数据是否相同
    /// \param lob_1 第一个LOB的指示器
    /// \param lob_2 第二个LOB的指示器
    /// \param amount 需要比较的数据长度，以字节为单位
    /// \param offset_1 第一个LOB要比较的的起始位置
    /// \param offset_2 第二个LOB要比较的起始位置
    bool COMPARE(LOBLocator *lob_1,LOBLocator *lob_2,uint64_t amount, uint64_t offset_1, uint64_t offset_2);
    /// 将目标一个LOB附加在另一个LOB后面
    /// \param dest_lob 被附加的目标LOB
    /// \param src_lob 要附加的源LOB
    Status APPEND(LOBLocator *dest_lob, LOBLocator *src_lob);
    /// 将src lob的指定数据copy到dest lob的指定偏移上
    /// \param dest_lob 目标LOB指示器
    /// \param src_lob 源LOB指示器
    /// \param amount 要复制的数据的长度
    /// \param dest_offset 目标LOB的指定偏移处
    /// \param src_offset 源LOB的指定偏移
    Status COPY(LOBLocator *dest_lob,LOBLocator *src_lob,uint64_t amount, uint64_t dest_offset, uint64_t src_offset);
    /// 从指定lob的offset处删除amout大小的数据，非原地更新
    /// \param lob_loc LOB的指示器
    /// \param amount 要删除的数据的长度
    /// \param offset 起始偏移
    Status ERASE(LOBLocator *lob_loc, uint64_t amount, uint64_t offset);
    /// 从指定lob的offset处删除amout大小的数据，原地更新
    /// \param lob_loc LOB的指示器
    /// \param amount 要删除的数据的长度
    /// \param offset 起始偏移
    Status FRAGMENT_DELETE(LOBLocator *lob_loc, uint64_t amount, uint64_t offset);
    /// 在LOB指定的偏移中插入一段数据，不影响后面的数据，原地更新
    /// \param lob_loc LOB的指示器
    /// \param amount 插入数据的长度
    /// \param offset 起始偏移
    /// \param data 要插入的数据
    Status FRAGMENT_INSERT(LOBLocator *lob_loc, uint64_t amount, uint64_t offset, const uint8_t *data);
    /// 将指定位置的数据移动到新的位置，原地更新
    /// \param lob_loc LOB的指示器
    /// \param amount 移动数据的长度
    /// \param src_offset 要移动数据的起始偏移
    /// \param dest_offset 目标偏移
    Status FRAGMENT_MOVE(LOBLocator *lob_loc, uint64_t amount, uint64_t src_offset, uint64_t dest_offset);
    /// 将旧的数据替换成buffer中新的数据
    /// \param lob_loc LOB的指示器
    /// \param old_amount 要被替换的数据的长度
    /// \param new_amount 插入数据的长度
    /// \param offset 替换数据的其实偏移
    /// \param data 新的数据
    Status FRAGMENT_REPLACE(LOBLocator *lob_loc, uint64_t old_amount, uint64_t new_amount, uint64_t offset, const uint8_t *data);
    /// 获取LOB数据的长度
    /// \param lob_loc LOB的指示器
    uint32_t GETLENGTH(LOBLocator *lob_loc){return lob_loc->get_data_size();}
    /// 从LOB指定的偏移处读取指定数量的数据
    /// \param lob_loc LOB的指示器
    /// \param amount 要读取的数据的长度
    /// \param offset 起始偏移
    /// \param data 输出数据
    Status READ(LOBLocator *lob_loc,uint64_t amount,uint64_t offset, uint8_t *data);
    /// 从LOB指定的偏移处写入指定数量的数据
    /// \param lob_loc LOB的指示器
    /// \param amount 要写入的数据的长度
    /// \param offset 起始偏移
    /// \param data 要写入的数据
    Status WRITE(LOBLocator *lob_loc,uint64_t amount,uint64_t offset, const uint8_t *data);
    /// 在LOB末尾追加数据
    /// \param lob_loc LOB的指示器
    /// \param amount 要追加多长的数据
    /// \param data 数据内容
    Status WRITEAPPEND(LOBLocator *lob_loc, uint64_t amount, const uint8_t *data);
    /// 将LOB裁剪到一定的大小
    /// \param lob_loc LOB的指示器
    /// \param newlen 要裁剪到的新的长度
    Status TRIM(LOBLocator *lob_loc, uint64_t newlen);
    
private:
    uint32_t new_lob_id();
};

#endif //fdsa