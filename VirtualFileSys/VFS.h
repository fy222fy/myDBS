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
    Segment(uint32_t id, DataFile* df);//初始化一个内存段
    ~Segment();
    //追加指定长度的数据
    Status append(uint32_t &offset, const vector<uint8_t> &data,uint32_t beg,uint32_t len);
    //在指定虚拟偏移写入指定数据
    Status write_page(uint32_t offset, const vector<uint8_t> &data,uint32_t beg,uint32_t len);
    //读取指定偏移的数据页
    Status read_page(uint32_t offset, vector<uint8_t> &data);
    //释放指定偏移的数据页
    Status free_page(uint32_t offset);
    //设置该段的第一块元数据信息的地址
    void set_meta_addr(BlockHandle *bh){
        meta_first_addr = bh->address;
    }
    //获得第一块元数据的地址
    void get_meta_addr(BlockHandle **bh){
        *bh = new BlockHandle(meta_first_addr);
    }
    static const uint32_t META_NUMS = BlockHead::FREE_SPACE / 8 - 1; //一个元数据文件中包含的地址数量，减一是因为还要存下一个元数据块的地址
    static const uint32_t PAGE_FREE_SPACE = BlockHead::FREE_SPACE; //页内可用空间大小
private:
    Options *op;//可选操作
    uint32_t ID; //段ID，由段名称生成而来，是唯一的
    uint32_t meta_first_addr;//第一块元数据块的地址
    DataFile *df;//数据文件指针
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

/**
 * 虚拟文件系统，负责数据文件的管理和段的分配
 * 类似Oracle的Tablespace
 * 1.0版本中只涉及一个数据文件
*/
class VFS{
public:
    static VFS *get_VFS(Options *op);//单例模式
    static VFS *get_VFS();//单例模式重载
    Status create_seg(uint32_t id, Segment **sh);//创建一个段
    Status open_seg(uint32_t id, Segment **sh);//打开一个段
    Status free_seg(uint32_t id);//删除一个段
    Status read_seg_head(BlockHandle *bh);//从bh中读出段头
    Status write_seg_head(BlockHandle *bh);//将段头写入bh中
    Status read_seg_meta(BlockHandle *bh, SegMeta **sm);//从bh中读取段meta
    Status write_seg_meta(BlockHandle *bh, SegMeta *sm);//在bh中写段meta
private:
    static VFS *vfs;//单例引用
    VFS();//没用的构造函数
    Status read_seg_info();//从文件头部读所有的段信息
    Status write_add_seg_info();//将段的头部信息全部写到数据文件头部
    Status free_all_page(BlockHandle *bh);//一个递归函数，为free_seg使用来释放所有的段用的
    vector<pair<uint32_t, uint32_t>> M;//保存所有段的id和对应的段头物理地址
    uint32_t nums;//目前段的数量
    bool is_active;//判断这个虚拟文件是否启用
    DataFile *df;
    const string fname = "datafile";//默认的文件名，后续支持多文件
};

#endif //