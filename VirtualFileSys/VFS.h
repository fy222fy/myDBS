/*******************************************
 * VFS虚拟文件系统
 * 向下操作数据文件模块进行文件操作
 * 向上提供一个自动增长的段
 * 用户只需要使用逻辑地址在段内进行操作即可
 ******************************************/
#ifndef VIRTUAL_FILE_SYS
#define VIRTUAL_FILE_SYS
#include "../include/status.h"
#include "../DataFile/DataFile.h"

struct SegsInfo;
struct SegMeta;


/**
 * 段类，提供操作这个段的各类方法
 * create_seg：创建一个新的段
 * open_seg：打开一个新的段
 * append：在段内写入一页数据，并获取写入的数据偏移
 * write_page：在指定的段中指定的页内写入数据
 * read_page：在指定段的指定页内读取数据
 * free_page：释放一个指定的页
*/
class Segment{
public:
    
    ~Segment();
    /** 
     * 在指定的默认数据文件名中以id创建一个段
     * id：段的唯一标识，不能重复
     * df：段对应的数据文件
     * op：可选操作
     * sh：要申请的段的指针的指针
    */
    static Status create_seg(uint32_t id, DataFile *df, Options *op, Segment **sh);
    /**
     * 打开一个已有的段空间
     * id：已经存在的段的id
     * df：段所在的数据文件
     * op：可选操作
     * sh：打开的段的指针的指针
    */
    static Status open_seg(uint32_t id, DataFile *df, Options *op, Segment **sh);
    /**
     * 在指段空间末尾写入一页数据，并获取写入的逻辑偏移，逻辑偏移是自动生成的
     * 如果想指定逻辑地址写入，请使用write函数
     * offset：逻辑地址，函数会自动寻找一个逻辑地址传出。
     * data：要写入的数据，不能大于一页
    */
    Status append(uint32_t &offset, const vector<uint8_t> &data);
    /**
     * 在指定的段空间的指定页处写入数据
     * offset：确定的逻辑地址（要在外层保证逻辑地址还没有使用过，否则出错）
     * data：要写入的数据
    */
    Status write_page(uint32_t offset, const vector<uint8_t> &data);
    /**
     * 在指定的段空间的指定页读取数据
     * offset：逻辑地址
     * data：读出的空间
    */
    Status read_page(uint32_t offset, vector<uint8_t> &data);
    /**
     * 在指定的段空间中指定的偏移处，释放一页
    */
    Status free_page(uint32_t offset);
    /**
     * 删除这个段，要修改段信息结构，删除段头以及释放所有段已经分配的空间
    */
    Status free_seg();
    static const uint32_t META_NUMS = BlockHead::FREE_SPACE / 8 - 1; //一个元数据文件中包含的地址数量，减一是因为还要存下一个元数据块的地址

private:
    Options *op;//可选操作

    uint32_t ID; //段ID，由段名称生成而来，是唯一的
    uint32_t meta_first_addr;//第一块元数据块的地址
    DataFile *df;//数据文件指针
    /**
     * 以df来初始化一个段，一个段目前对应一个数据文件
    */
    Segment(uint32_t id, DataFile* df, Options *options);
    /**
     * 从数据文件头部读取所有的段的信息（就一个块），内容是段的逻辑id和段头地址对应关系
     * sis：段信息结构，一个数据文件一个
    */
    Status read_seg_info(SegsInfo **sis);
    /**
     * 在数据文件头写入段信息，一般在创建段或删除段的时候使用
    */
    Status write_add_seg_info(SegsInfo *sis);
    /**
     * 从bh所指向的块中以段头结构读取段头结构（一个块）
    */
    Status read_seg_head(BlockHandle *bh);
    /**
     * 将新的段头信息写在bh指定的块中
    */
    Status write_seg_head(BlockHandle *bh);
    /**
     * 从bh指定的块中读取段的一段元数据信息
     * sm：段的元数据信息
    */
    Status read_seg_meta(BlockHandle *bh, SegMeta **sm);
    /**
     * 将元数据信息写在bh指定的块中
     * sm：段的元数据信息
    */
    Status write_seg_meta(BlockHandle *bh, SegMeta *sm);
    //设置该段的第一块元数据信息的地址
    void set_meta_addr(BlockHandle *bh){
        meta_first_addr = bh->address;
    }
    //获得第一块元数据的地址
    void get_meta_addr(BlockHandle **bh){
        *bh = new BlockHandle(meta_first_addr);
    }
    //一个递归函数，为free_seg使用来释放所有的段用的
    Status free_all_page(BlockHandle *bh);

};

/**
 * 段们的信息，包括有哪些段，段的物理地址在哪
*/
struct SegsInfo{
    vector<pair<uint32_t, uint32_t>> M;//保存段对应的信息
    ~SegsInfo(){
        vector<pair<uint32_t, uint32_t>>().swap(M);//清空vector
    }
};

/**
 * 段的元数据信息，与段元数据信息磁盘结构一致
 * if_have_next：是否还有下一个元数据块
 * nex_meta_addr：下一个元数据块的地址
 * M：META_NUMS组按逻辑顺序的页信息
 *  每组： | 状态（是否使用） |   物理地址   |
*/
struct SegMeta{
    vector<pair<uint32_t, uint32_t>> M;//保存段对应的信息
    uint32_t if_have_next;//是否还有下一个地址？是则是1，不是则是0
    uint32_t next_meta_addr;
    SegMeta():next_meta_addr(0),if_have_next(0x00){
        M.resize(Segment::META_NUMS,make_pair(0x00,0x00));
    }
    ~SegMeta(){
        vector<pair<uint32_t, uint32_t>>().swap(M);//清空vector
    }
};

#endif //