/**
 * 标准文件接口
 * */

#ifndef OS_FILE_OPERATION
#define OS_FILE_OPERATION
#include<string>
#include<vector>
#include <stdint.h>
#include "../include/status.h"

/**
 * 一个文件操作类，该类操作一个文件
 * 实现打开、读、写、关闭等操作
*/
class RWFile{
public:
    RWFile(){}
    virtual ~RWFile(){}
    /**
     * 以指定文件名打开一个文件
    */
    virtual Status Open(std::string fname)=0;
   /**
     * 在指定偏移处，从文件中读取n长度的数据
     * offset：指定的偏移，以字节为单位B
     * n：读取的长度
     * result：提取的数组
    */
    virtual Status Read(uint64_t offset, size_t n, std::vector<uint8_t> &result)=0;
    /**
     * 在文件末尾附加数据
    */
    virtual Status Append(const std::vector<uint8_t> &data)=0;
    /**
     * 在文件指定偏移写入数据
     * offset：指定偏移，以字节为单位B
     * data：提取的数组
    */
    virtual Status Write(uint64_t offset, const std::vector<uint8_t> &data, uint32_t beg, uint32_t len) = 0;
    /**
     * 将写入的文件刷到磁盘
    */
    virtual Status Flush()=0;
    /**
     * 同步
    */
    virtual Status Sync()=0;
    /**
     * 关闭文件
    */
    virtual void Close()=0;
    /**
     * 获取文件长度
     * 返回值：文件大小，字节
    */
    virtual uint64_t Size()=0;
};

/**
 * 通用操作系统接口
 * 提供创建文件，删除文件等操作
*/
class Env{
public:
    Env(){};
    /**
     * 给定文件名和RWFILE文件指针创建一个文件
     * filename：文件名
     * f：RW数据文件指针的指针
    */
    virtual Status NewFile(const std::string &filename, RWFile **f)=0;
    /**
     * 给定文件名打开一个文件
     * filename：文件名
     * f：RW数据文件指针的指针
    */
    virtual Status OpenFile(const std::string &filename, RWFile **f) = 0;
    /**
     * 给定文件名删除一个文件
    */
    virtual Status DeleteFile(const std::string &filename) = 0;
};

#endif /**/