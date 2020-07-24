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

struct SegHead;
struct SegMeta;
/**
 * 虚拟文件系统，负责数据文件的管理和段的分配
 * 类似Oracle的Tablespace
 * 1.0版本中只涉及一个数据文件
*/
class VFS{
public:
    static VFS *get_VFS(Options *op);//单例模式
    static VFS *get_VFS();//单例模式重载
    Status create_seg(uint32_t id);//创建一个段
    Status free_seg(uint32_t id);//删除一个段
    //追加指定长度的数据
    Status append(uint32_t id, uint32_t &offset, const vector<uint8_t> &data,uint32_t beg,uint32_t len);
    //在指定虚拟偏移写入指定数据
    Status write_page(uint32_t id, uint32_t offset, const vector<uint8_t> &data,uint32_t beg,uint32_t len);
    //读取指定偏移的数据页
    Status read_page(uint32_t id, uint32_t offset, vector<uint8_t> &data);
    //释放指定偏移的数据页
    Status free_page(uint32_t id, uint32_t offset);
    //获取一个没用过的段ID
    uint32_t new_segid();
    static const uint32_t META_NUMS = BlockHead::FREE_SPACE / 8 - 1; //一个元数据文件中包含的地址数量，减一是因为还要存下一个元数据块的地址
    static const uint32_t PAGE_FREE_SPACE = BlockHead::FREE_SPACE; //页内可用空间大小
private:
    static VFS *vfs;//单例引用
    Options *op;
    VFS(Options *op);//没用的构造函数
    Status read_seg_info();//从文件头部读所有的段信息
    Status write_add_seg_info();//将段的头部信息全部写到数据文件头部
    Status read_seg_head(BlockHandle *bh, SegHead **sh);//从bh中读出段头
    Status write_seg_head(BlockHandle *bh, SegHead *sh);//将段头写入bh中
    Status read_seg_meta(BlockHandle *bh, SegMeta **sm);//从bh中读取段meta
    Status write_seg_meta(BlockHandle *bh, SegMeta *sm);//在bh中写段meta
    Status free_all_page(BlockHandle *bh);//一个递归函数，为free_seg使用来释放所有的段用的
    map<uint32_t, uint32_t> M;//保存所有段的id和对应的段头物理地址
    uint32_t nums;//目前段的数量
    bool is_active;//判断这个虚拟文件是否启用
    DataFile *df;
    const string fname = "datafile";//默认的文件名，后续支持多文件
};

/**
 * 段头结构
*/
struct SegHead{
public:
    uint32_t ID;//段的ID,一般也就是表的ID
    uint32_t meta_first_addr;//第一块元数据块的地址
    //内存段头创建，之后调用write写入
    SegHead(uint32_t id, uint32_t addr)
        :ID(id), meta_first_addr(addr){}
    //空创建，必须调用read读入
    SegHead():ID(0), meta_first_addr(0){}
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
    //空创建，可以用于未来从文件读入，也可以直接写，默认没有什么必要信息
    SegMeta():next_meta_addr(0),if_have_next(0x00){
        M.resize(VFS::META_NUMS,make_pair(0x00,0x00));
    }
    ~SegMeta(){
        vector<pair<uint32_t, uint32_t>>().swap(M);//清空vector
    }
};



#endif //