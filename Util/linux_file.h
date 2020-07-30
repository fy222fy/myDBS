#ifndef LINUX_FILE
#define LINUX_FILE
#include "../include/env.h"
#include "../include/status.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
using namespace std;
/**
 * 在linux环境下实现读写文件的文件操作类，继承自RWFile
*/
class LinuxFile: public RWFile{
private:
    std::string filename;
    int fd;
public:
    //在不打开文件流的情况下创建一个空对象，在此之后必须执行open函数
    LinuxFile();
    //文件流未打开，使用该文件名创建一个RWfile对象
    LinuxFile(const std::string &fname);
    //正常使用该构造函数，给定文件流指针
    LinuxFile(const std::string &fname, int fd);
    virtual ~LinuxFile();
    /**
     * 以指定文件名打开一个文件
    */
    virtual Status Open(std::string fname);
    /**
     * 在指定偏移处，从文件中读取n长度的数据
     * offset：指定的偏移，以字节为单位B
     * n：读取的长度
     * result：提取的数组
    */
    virtual Status Read(uint64_t offset, size_t n, uint8_t *result);
    /**
     * 在文件末尾附加数据
    */
    virtual Status Append(const uint8_t *data);
    /**
     * 在文件指定偏移写入数据
     * offset：指定偏移，以字节为单位B
     * data：提取的数组
    */
    virtual Status Write(uint64_t offset, const uint8_t *result, uint32_t len);
    /**
     * 将写入的文件刷到磁盘
    */
    virtual Status Flush();
    /**
     * 同步
    */
    virtual Status Sync();
    /**
     * 关闭文件
    */
    virtual void Close();
    /**
     * 获取文件长度
     * 返回值：文件大小，字节
    */
    virtual uint64_t Size();
};

class LinuxEnv: public Env{
public:
    LinuxEnv(){}
    /**
     * 给定文件名和RWFILE文件指针创建一个文件
     * filename：文件名
     * f：RW数据文件指针的指针
    */
    virtual Status NewFile(const std::string &filename, RWFile **f);
    /**
     * 给定文件名打开一个文件
     * filename：文件名
     * f：RW数据文件指针的指针
    */
    virtual Status OpenFile(const std::string &filename, RWFile **f);
    /**
     * 给定文件名删除一个文件
    */
    virtual Status DeleteFile(const std::string &filename);
    /**
     * 给定文件名确定文件是否存在
    */
    virtual bool if_exsist(const std::string &filename);

private:
    
};

#endif 