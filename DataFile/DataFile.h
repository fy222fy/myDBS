/*******************************************************
 * 数据文件模块
 * 功能：调用系统提供的文件操作，像上层提供数据文件和宏块
*******************************************************/
#ifndef OB_SSTABLE
#define OB_SSTABLE
#include <string>
#include <fstream>
#include <vector>
#include <array>
#include <map>
#include "../include/env.h"
#include "../include/status.h"
#include "../include/option.h"
#include "../Util/Util.h"
#include "Block.h"
using namespace std;


/**
 * 数据文件结构，提供打开数据文件、创建数据文件、分配块、写块、读块、释放块等操作。
*/
struct DataFile{ //数据文件的结构
public:
    const static uint32_t HEAD_SIZE = 16; //设置头部的初始大小
    const static uint32_t DF_MAX_SIZE = 1000000;//设置数据文件最大长度
    /**
     * 如果当前还不存在数据文件，调用该函数
     * 以给定文件名创建一个数据文件，并返回该数据文件指针
    */
    static Status open_datafile(string fname, Options *options, DataFile **df);
    /**
     * 如果当前还不存在数据文件，调用该函数
     * 以给定文件名创建一个数据文件，并返回该数据文件指针
    */
    static Status create_datafile(string fname, Options *options, DataFile **df);
    /**
     * 数据文件操作完毕后，如写入数据完成
     * 则关闭文件指针等资源
    */
    ~DataFile();
    /**
     * 申请一个块句柄给用户，句柄中的核心数据是物理地址
     * 此时还没有写文件，只是把文件的这一块给预留出来了
    */
    Status alloc_block(BlockHandle **bb);
    /**
     * 将块内容给free掉，只是修改文件中对应块头的状态位
     * 还有修改内存中的空闲链表
     * 这里为了保证持久化，可能需要增加log文件以记录修改过程
    */
    Status free_block(BlockHandle *bb);
    /**
     * 向已经分配的块中写入一段数据
     * 这里不做缓存，直接写入文件的对应位置
     * 除了上层用户写入的数据以外，还要把块头信息一起写入
    */
    Status write_block(const vector<uint8_t> &data, uint32_t beg, uint32_t len, BlockHandle *bb);
    /**
     * 将缓冲区内的所有数据刷到磁盘上
     * 即调用文件接口的函数写文件
    */
    Status flush();
    /**
     * 根据给定的物理地址，读取一个块到内存中并返回
     * 注意这里是从文件中读取的
    */
    Status read_block(BlockHandle *bh, vector<uint8_t> &result);
    //为第一次打开文件的用户提供一个首要数据的偏移
    Status get_first_bh(BlockHandle **bh);
    //获得当前地址的下一个地址，这两个函数今后不一定使用
    Status get_next_bh(BlockHandle **bh);
    //关闭文件
    void Close();
private:
    RWFile *file;//数据文件对应真实文件的指针，从env中打开
    string filename;//文件名
    uint32_t ID;//数据文件唯一编号
    uint8_t type;//数据文件类型，目前只考虑普通用户数据文件
    map<uint32_t,bool> *free_map; //块空闲位图，块地址-是否空闲，这是从文件中读出来的    
    //以文件指针和文件名来构造数据文件对象
    DataFile(RWFile *f, string fname);
    //初始化空闲位图
    Status init_map();
    //将头部信息序列化
    Status serialize_head(vector<uint8_t> &result);
    //将序列化好的头部信息反序列化成头部状态
    Status deserialize_head(const vector<uint8_t> &input);
    //从文件中读取头部数据
    Status read_head();
    //将头部数据写入头部
    Status write_head();
    //从指定地址中读取块的头部信息
    Status read_block_head(BlockHandle *bh, BlockHead **bhead);
    //将块头部信息写入指定地址
    Status write_block_head(BlockHandle *bh, BlockHead *bhead);
};//



#endif //