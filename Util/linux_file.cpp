
#include "../include/env.h"
#include "linux_file.h"
#include "../include/status.h"

#include <string>
#include <vector>

#include <error.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
LinuxFile::LinuxFile():filename(""),fd(-1){}

LinuxFile::LinuxFile(const std::string &fname,int ffd)
    :filename(fname),fd(ffd){}

LinuxFile::~LinuxFile(){
    close(fd);
}

/**
 * 以指定文件名打开一个文件
*/
Status LinuxFile::Open(std::string fname){ 
    Status s;
    filename = fname;
    if(fd != -1) s.FetalError("该数据文件已经打开，使用有误。");
    fd = open(filename.c_str(),O_RDWR);
    if(fd == -1){
        s.FetalError("文件打开失败，可能是还没有创建文件");
    } 
    return s;
}
/*
 * 在指定偏移处，从文件中读取n长度的数据
*/
Status LinuxFile::Read(uint64_t offset, size_t n, uint8_t *result){
    Status s;
    if(fd == -1){
        s.FetalError("系统读取错误，文件还未打开");
    }
    lseek(fd,offset,SEEK_SET);
    int ret = read(fd,result,n);
    if(ret == 0) s.FetalError("系统读取错误，读到文件尾");
    if(ret == -1) s.FetalError("系统读取错误");
    return s;
}
/**
 * 附加数据
*/
Status LinuxFile::Append(const uint8_t *data){
    Status s;
    s.FetalError("该函数还未实现，请勿调用");
    return s;
}

Status LinuxFile::Write(uint64_t offset, const uint8_t *data, uint64_t len){
    Status s;
    if(fd == -1){
        s.FetalError("系统写入错误，文件还未打开");     
    }
    lseek(fd,offset,SEEK_SET);//将文件指针移动到offset处
    ssize_t write_len = write(fd,data,len);
    if(write_len != len) s.FetalError("系统写入出错，可能是磁盘空间已满或者是超出文件最大限制");
    return s;
}

/**
 * 刷新
*/
Status LinuxFile::Flush(){
    Status s;
    if(fsync(fd) != 0) s.FetalError("文件sync失败");
    return s;
}
/**
 * 同步
*/
Status LinuxFile::Sync(){
    Status s;
    sync();
    return s;
}

/**
 * 关闭文件
*/
void LinuxFile::Close(){
    close(fd);
}

uint64_t LinuxFile::Size(){
    return lseek(fd,0,SEEK_END);
}


/////下面是Env的实现

Status LinuxEnv::NewFile(const std::string &filename, RWFile **f){
    Status s;
    int fd = open(filename.c_str(),O_CREAT|O_EXCL|O_RDWR,S_IRWXU);
    if(fd == -1) s.FetalError("文件已经存在，请检查使用");
    *f = new LinuxFile(filename.c_str(),fd);
    return s;
}

Status LinuxEnv::OpenFile(const std::string &filename, RWFile **f){
    Status s;
    *f = new LinuxFile();
    s = (*f)->Open(filename);
    return s;
}

Status LinuxEnv::DeleteFile(const std::string &filename){
    Status s;
    int ret = remove(filename.c_str());
    if(ret == -1) s.FetalError("文件删除失败");
    return s;
}

bool LinuxEnv::if_exsist(const std::string &filename){
    std::ifstream fin(filename);
    if(fin) return true;
    return false;
}

