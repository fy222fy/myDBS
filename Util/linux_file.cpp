
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
LinuxFile::LinuxFile():filename(""),file(NULL){}
LinuxFile::LinuxFile(const std::string &fname)
    :filename(fname){
    file = new std::fstream();
    file->open(fname, std::ios::in|std::ios::out|std::ios::binary);
    assert(file && "文件打开失败");
}

LinuxFile::LinuxFile(const std::string &fname,std::fstream *f)
    :filename(fname),file(f){}

LinuxFile::~LinuxFile(){
    file->close();
    delete file;
}

/**
 * 以指定文件名打开一个文件
*/
Status LinuxFile::Open(std::string fname){ 
    Status s;
    filename = fname;
    std::ifstream fin(fname);
    if(!fin) s.FetalError("文件还不存在就调用了系统的打开文件操作，请先使用env创建文件");
    if(file) s.FetalError("如果使用open函数，请不要使用带文件指针f的构造函数");
    fin.close();
    file = new std::fstream(fname, std::ios::in|std::ios::out|std::ios::binary);
    if(!file) s.FetalError("系统错误，文件打开失败");
    return s;
}
/*
 * 在指定偏移处，从文件中读取n长度的数据
*/
Status LinuxFile::Read(uint64_t offset, size_t n, std::vector<uint8_t> &result){
    Status s;
    if(result.size() != 0) result.clear();
    file->clear();
    file->seekg(offset,std::ios::beg);
    char temp[n];
    file->read(temp,n);
    result.insert(result.begin(),temp,temp+n);
    if(!file) s.FetalError("文件重定位错误或写入错误");
    return s;
}
/**
 * 附加数据
*/
Status LinuxFile::Append(const std::vector<uint8_t> &data){
    Status s;
    s.FetalError("该函数还未实现，请勿调用");
    return s;
}

Status LinuxFile::Write(uint64_t offset, const std::vector<uint8_t> &data){
    Status s;
    if(!(*file)){
        file->open(filename.c_str(),std::ios::in|std::ios::out|std::ios::binary);
        if(!*file) s.FetalError("系统错误，文件打开失败");     
    }
    file->clear();
    file->seekp(offset,std::ios::beg);
    for(uint8_t item:data){
        *file << item;
    }
    if(!*file) s.FetalError("文件写入失败");
    return s;
}

/**
 * 刷新
*/
Status LinuxFile::Flush(){
    Status s;
    *file << std::flush;
    if(!*file) s.FetalError("文件flush失败");
    return s;
    /*
    Status s;
    if(fflush(this->file) != 0) s = SError("文件flush失败");
    if(fsync(fileno(this->file)) != 0) s = SError("文件sync失败");
    return s;
    */
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
    file->close();
}

uint64_t LinuxFile::Size(){
    file->seekg(0,file->end);
    return file->tellg();
}


/////下面是Env的实现

Status LinuxEnv::NewFile(const std::string &filename, RWFile **f){
    Status s;
    std::ifstream fin(filename);
    if(fin) s.Warning("文件已经存在，忽略该问题并覆盖该文件");
    fin.close();
    std::fstream *fs = new std::fstream(filename, std::ios::out|std::ios::binary);
    if(!(*fs)) s.FetalError("文件在创建后没有成功打开");
    fs->close();
    fs = new std::fstream(filename, std::ios::in|std::ios::out|std::ios::binary);
    *f = new LinuxFile(filename, fs);
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

