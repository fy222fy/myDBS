#ifndef OB_LOB_INTERFACE
#define OB_LOB_INTERFACE
#include<string>
#include<vector>
#include "../include/status.h"
#include "LOB_locator.h"
#include "../Util/Util.h"
struct LOBH;
struct LHP;
struct LHIP;


/**
 * LOB Page的结构
 * size：大小
 * checksum：校验和
 * type：这个页的作用，主要是确定一阶索引还是二阶索引
*/
struct LOBH
{
    LOBH(uint32_t s,uint32_t cks):size(s),checksum(cks),type(0x00){}
    LOBH():size(0),checksum(0),type(0x00){}
    ~LOBH(){}
    uint32_t size;
    uint32_t checksum;
    uint8_t type;
    static const uint32_t HEAD_SIZE = 8;//LOB页头结构的大小
    void Serialize(vector<uint8_t> &output){
        uint8_t *temp = int32_to_int8(size);
        for(int i = 0; i < 4;i++) output.emplace_back(temp[i]);
        temp = int32_to_int8(checksum);
        for(int i = 0; i < 4;i++) output.emplace_back(temp[i]);
    }
    void Deserialize(vector<uint32_t> &input){

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
    LHP():nums(0),checksum(1279807488),type(0x01){}
    ~LHP(){vector<uint32_t>().swap(M);}
    uint32_t nums;
    uint32_t checksum;
    uint8_t type;//本LHP是用来做什么的
    vector<uint32_t> M;
    static const uint32_t HEAD_SIZE = 8;
    static const uint32_t MAX_LPA = (VFS::PAGE_FREE_SPACE - HEAD_SIZE) / 4;
    void append(uint32_t lpa){
        M.emplace_back(lpa);
        nums++;
    }//加入一个lpa
    void Serialize(uint8_t *result){
        uint8_t *temp = int32_to_int8(nums);
        for(int i = 0; i < 4;i++) result.emplace_back(temp[i]);
        temp = int32_to_int8(checksum);
        for(int i = 0; i < 4;i++) result.emplace_back(temp[i]);
        for(int j = 0; j < nums; j++){
            temp = int32_to_int8(M[j]);
            for(int i = 0; i < 4;i++) result.emplace_back(temp[i]);
        }
    }
    void Deserialize(const uint8_t *input){
        int it = 0;
        nums = int8_to_int32(input,it);
        it+=4;
        checksum = int8_to_int32(input,it);
        it+=4;
        for(;it<input.size();it=it+4){
            M.emplace_back(int8_to_int32(input,it));
        }
    }
};

/**
 * LOB Header Page Index的结构
 * size：大小
 * checksum：校验和
 * type：这个页的作用，主要是确定一阶索引还是二阶索引
*/
struct LHIP
{
    LHIP():nums(0),checksum(1279805776),type(0x02){}
    ~LHIP(){vector<uint32_t>().swap(M);}
    uint32_t nums;
    uint32_t checksum;
    uint8_t type;//类型
    vector<uint32_t> M;
    static const uint32_t HEAD_SIZE = 8;
    void append(uint32_t lhpa){
        M.emplace_back(lhpa);
        nums++;
    }//加入一个lpa
    uint32_t read_last_lhpa(){
        return *(M.end() - 1);
    }
    void Serialize(vector<uint8_t> &result){
        uint8_t *temp = int32_to_int8(nums);
        for(int i = 0; i < 4;i++) result.emplace_back(temp[i]);
        temp = int32_to_int8(checksum);
        for(int i = 0; i < 4;i++) result.emplace_back(temp[i]);
        for(int j = 0; j < nums; j++){
            temp = int32_to_int8(M[j]);
            for(int i = 0; i < 4;i++) result.emplace_back(temp[i]);
        }
    }
    void Deserialize(const vector<uint8_t> &input){
        int it = 0;
        nums = int8_to_int32(input,it);
        it+=4;
        checksum = int8_to_int32(input,it);
        it+=4;
        for(;it<input.size();it=it+4){
            append(int8_to_int32(input,it));
        }
    }
};
///LOB模块，负责LOB的读写等操作，主要是根据locator操作
///
///
class LOBimpl{
public:
    LOBimpl();
    Status create_locator(LOBLocator **llp, uint32_t seg_id);//创建一个空的lob，获取locator
    Status create_lobseg(uint32_t &seg_id);//创建一个lob段，获取段的id
    Status append(LOBLocator *ll, const std::vector<uint8_t> &data);//追加数据，并修改locator结构
    Status write(LOBLocator *ll, uint32_t data_off, const std::vector<uint8_t> &data);//覆写数据
    Status read(LOBLocator *ll, uint32_t amount, uint32_t data_off, std::vector<uint8_t> &result);//读取数据
    Status erase(LOBLocator *ll, uint32_t amount, uint32_t data_off);//删除部分数据
    Status drop(LOBLocator *ll);//删除指向的所有lob数据
    static const uint32_t LOB_PAGE_SIZE = VFS::PAGE_FREE_SPACE - LOBH::HEAD_SIZE;
    static const uint32_t LHP_SIZE = VFS::PAGE_FREE_SPACE - LOBH::HEAD_SIZE;
    static const uint32_t LHP_NUMS = LHP_SIZE / 4;
    static const uint32_t LHPI_SIZE = VFS::PAGE_FREE_SPACE - LOBH::HEAD_SIZE;
    static const uint32_t LHPI_NUMS = LHPI_SIZE / 4;
    static const uint32_t OUTLINE_1_MAX_SIZE = LOBLocator::MAX_LPA * LOB_PAGE_SIZE;//行外存1模式最大数据量
    static const uint32_t OUTLINE_2_MAX_SIZE = LHP_NUMS * LOB_PAGE_SIZE;
    static const uint32_t OUTLINE_3_MAX_SIZE = LHPI_NUMS * LHP_NUMS * LOB_PAGE_SIZE;
private:
    Options *op;
    uint32_t checksum(LOBLocator *ll);//校验和计算
    //在指定段上创建一个lob页，然后写入指定长度的数据 
    Status create_lobpage(uint32_t segid, uint32_t &offset, const vector<uint8_t> &data, uint32_t beg, uint32_t len);
    //Status append_lobpage(uint32_t &offset, LOBP *lobp);
    Status write_lobpage(uint32_t segid, uint32_t offset, const vector<uint8_t> &data, uint32_t beg, uint32_t len);
    Status read_lobpage(uint32_t segid, uint32_t offset, vector<uint8_t> &output);
    Status free_lobpage(uint32_t offset);
    Status create_LHP(LHP **lhp);
    //Status append_LHP(uint32_t &offset, LHP *lhp);
    Status write_LHP(uint32_t segid, uint32_t &offset, LHP *lhp);
    Status read_LHP(uint32_t segid, uint32_t offset, LHP **lhp);
    Status free_LHP();
    Status create_LHIP(LHIP **lhip);
    //Status append_LHIP(uint32_t &offset, LHIP *lhip);
    Status write_LHIP(uint32_t segid, uint32_t &offset, LHIP *lhip);
    Status read_LHIP(uint32_t segid, uint32_t offset, LHIP **lhip);
    Status free_LHIP();
    uint32_t new_lob_id();
    //在out-1结构中插入数据，如果能全部插入就全部插入，否则插满返回即可
    Status insert_out1(LOBLocator *ll,const vector<uint8_t> &data, uint32_t &beg);
};

//LOB接口
class LOB{
public:
    LOB(){}
    /// 比较两个LOB中的一段数据是否相同
    /// \param lob_1 第一个LOB的指示器
    /// \param lob_2 第二个LOB的指示器
    /// \param amount 需要比较的数据长度，以字节为单位
    /// \param offset_1 第一个LOB要比较的的起始位置
    /// \param offset_2 第二个LOB要比较的起始位置
    Status COMPARE(LOBLocator *lob_1,LOBLocator *lob_2,uint32_t amount, uint32_t offset_1, uint32_t offset_2);
    /// 将目标一个LOB附加在另一个LOB后面
    /// \param dest_lob 被附加的目标LOB
    /// \param src_lob 要附加的源LOB
    Status APPEND(LOBLocator *dest_lob, LOBLocator src_lob);
    /// 将src lob的指定数据copy到dest lob的指定偏移上
    /// \param dest_lob 目标LOB指示器
    /// \param src_lob 源LOB指示器
    /// \param amount 要复制的数据的长度
    /// \param dest_offset 目标LOB的指定偏移处
    /// \param src_offset 源LOB的指定偏移
    Status COPY(LOBLocator *dest_lob,LOBLocator *src_lob,uint32_t amount, uint32_t dest_offset, uint32_t src_offset);
    /// 从指定lob的offset处删除amout大小的数据，非原地更新
    /// \param lob_loc LOB的指示器
    /// \param amount 要删除的数据的长度
    /// \param offset 起始偏移
    Status ERASE(LOBLocator *lob_loc, uint32_t amount, uint32_t offset);
    /// 从指定lob的offset处删除amout大小的数据，原地更新
    /// \param lob_loc LOB的指示器
    /// \param amount 要删除的数据的长度
    /// \param offset 起始偏移
    Status FRAGMENT_DELETE(LOBLocator *lob_loc, uint32_t amount, uint32_t offset);
    /// 在LOB指定的偏移中插入一段数据，不影响后面的数据，原地更新
    /// \param lob_loc LOB的指示器
    /// \param amount 插入数据的长度
    /// \param offset 起始偏移
    /// \param data 要插入的数据
    Status FRAGMENT_INSERT(LOBLocator *lob_loc, uint32_t amount, uint32_t offset, const vector<uint8_t> &data);
    /// 将指定位置的数据移动到新的位置，原地更新
    /// \param lob_loc LOB的指示器
    /// \param amount 移动数据的长度
    /// \param src_offset 要移动数据的起始偏移
    /// \param dest_offset 目标偏移
    Status FRAGMENT_MOVE(LOBLocator *lob_loc, uint32_t amount, uint32_t src_offset, uint32_t dest_offset);
    /// 将旧的数据替换成buffer中新的数据
    /// \param lob_loc LOB的指示器
    /// \param old_amount 要被替换的数据的长度
    /// \param new_amount 插入数据的长度
    /// \param offset 替换数据的其实偏移
    /// \param data 新的数据
    Status FRAGMENT_REPLACE(LOBLocator *lob_loc, uint32_t old_amount, uint32_t new_amount, uint32_t offset, const vector<uint8_t> &data);
    /// 获取LOB数据的长度
    /// \param lob_loc LOB的指示器
    uint32_t GETLENGTH(LOBLocator *lob_loc){return lob_loc->data_size;}
    /// 从LOB指定的偏移处读取指定数量的数据
    /// \param lob_loc LOB的指示器
    /// \param amount 要读取的数据的长度
    /// \param offset 起始偏移
    /// \param data 输出数据
    Status READ(LOBLocator *lob_loc,uint32_t amount,uint32_t offset, vector<uint8_t> &data);
    /// 从LOB指定的偏移处写入指定数量的数据
    /// \param lob_loc LOB的指示器
    /// \param amount 要写入的数据的长度
    /// \param offset 起始偏移
    /// \param data 要写入的数据
    Status WRITE(LOBLocator *lob_loc,uint32_t amount,uint32_t offset, const vector<uint8_t> &data);
    /// 在LOB末尾追加数据
    /// \param lob_loc LOB的指示器
    /// \param amount 要追加多长的数据
    /// \param data 数据内容
    Status WRITEAPPEND(LOBLocator *lob_loc, uint32_t amount, const vector<uint8_t> &data);
    /// 将LOB裁剪到一定的大小
    /// \param lob_loc LOB的指示器
    /// \param newlen 要裁剪到的新的长度
    Status TRIM(LOBLocator *lob_loc, uint32_t newlen);
    
private:
    LOBimpl lobimpl;
};

#endif //fdsa