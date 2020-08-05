/****************************************************
 * 宏块模块
 * 功能：定义 宏块块头结构，宏块句柄
*******************************************************/
#ifndef OB_BLOCK
#define OB_BLOCK

#include <stdint.h>
#include<iostream>
#include<fstream>
#include "../include/env.h"
#include "../Util/Util.h"

/**
 * 宏块块头的结构，该结构与磁盘上的宏块结构对应。
 * if_free：表示该块是否空闲
 * used_space：该块中的数据占位多少
 * block_size: 该块整体的大小，这部分是固定的
 * in_offset：数据在宏块中的偏移，也就是块头的大小，这里目前也是固定的
 * 
 * MAX_SIZE：宏块的大小
 * HEAD_SIZE：块头大小
 * FREE_SPACE：数据块的最大大小
*/
struct BlockHead{
    bool if_free;//该块是否空闲
    uint32_t used_space;//已经使用过的空间
    uint32_t block_size;//块的整体大小，这个部分是固定的
    uint32_t in_offset; //数据在该宏块中的偏移，最初这里设置是固定的。

    const static uint32_t MAX_SIZE = 160;
    const static uint32_t HEAD_SIZE = 16;
    const static uint32_t FREE_SPACE = MAX_SIZE - HEAD_SIZE;
    Status Deserialize(const uint8_t *result){
        Status s;
        Convert convert;
        if_free = result[0] == 0x02 ? false:true;
        used_space = convert.int8_to_int32(result, 1);
        block_size = convert.int8_to_int32(result, 5);
        in_offset = convert.int8_to_int32(result,9);
        return s;
    }
    Status Serialize(uint8_t *data){
        Status s;
        Convert convert;
        data[0] = if_free == true ? 0x01 : 0x02;
        uint8_t *temp = convert.int32_to_int8(used_space);
        data[1] = temp[0];
        data[2] = temp[1];
        data[3] = temp[2];
        data[4] = temp[3];
        temp = convert.int32_to_int8(block_size);
        data[5] = temp[0];
        data[6] = temp[1];
        data[7] = temp[2];
        data[8] = temp[3];
        temp = convert.int32_to_int8(in_offset);
        data[9] = temp[0];
        data[10] = temp[1];
        data[11] = temp[2];
        data[12] = temp[3];
        return s;
    }
    //构造函数创建一个还没有使用过的空块，参数是这个块是否使用过
    BlockHead()
        :if_free(false),
        used_space(0),
        block_size(MAX_SIZE),
        in_offset(HEAD_SIZE){}
    ~BlockHead(){}
    void set_free(){if_free = true;}
};
/**
 * 宏块的句柄，包含宏块的物理地址，用来对一个块进行操作
*/
class BlockHandle{
public:
    /**
     * 用一个已有的地址来初始化这个handle
    */
    BlockHandle(uint64_t addr):address(addr){}
    BlockHandle():address(0){}
    ~BlockHandle(){}
    uint64_t address; //物理地址
};

#endif /**/