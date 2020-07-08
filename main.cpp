#include<iostream>
#include<fstream>
#include<string>
#include "include/status.h"
#include "include/env.h"
#include "Util/linux_file.h"
#include "DataFile/Block.h"
#include "DataFile/DataFile.h"
#include "VirtualFileSys/VFS.h"
#include <ctime>
using namespace std;

RWFile *new_linuxf(){
    string fname = "333";
    RWFile *RR;
    Env *env = new LinuxEnv();
    env->NewFile(fname, &RR);
    delete env;
    return RR;
}


int test_env_newandwritefile(){
    RWFile *RR = new_linuxf();
    vector<uint8_t> v{0x41,0x42,0x43,0x44};
    Status s = RR->Write(20,v);
    RR->Flush();
    if(!s.isok()) cout<< "失败" + s.err_msg() <<endl;
    else cout<< "成功" <<endl;
    RR->Close();
    return 0;
}

RWFile *get_linuxf(){
    string fname = "333";
    RWFile *RR;
    Env *env = new LinuxEnv();
    env->OpenFile(fname, &RR);
    delete env;
    return RR;
}

int test_env_readfile(){
    RWFile *RR = get_linuxf();
    vector<uint8_t> v;
    Status s = RR->Read(20,3,v);
    if(!s.isok()) cout<< "失败" + s.err_msg() <<endl;
    else cout<< "成功" <<endl;
    for(uint8_t item:v){
        cout << unsigned(item) << endl;
    }
    delete RR;
    return 0;
}

int create_adatafile(){
    Env *env = new LinuxEnv();
    Options *op = new Options(env);
    DataFile *df;
    DataFile::create_datafile("myFile",op,&df);
    delete df;
    delete op;
}

int open_adatafile(){
    Env *env = new LinuxEnv();
    Options *op = new Options(env);
    DataFile *df;
    DataFile::open_datafile("myFile",op,&df);
    delete df;
    delete op;
}

int test_alloc_block(){
    Env *env = new LinuxEnv();
    Options *op = new Options(env);
    DataFile *df;
    DataFile::open_datafile("myFile",op,&df);
    BlockHandle *bb;
    df->alloc_block(&bb);
    vector<uint8_t> data(BlockHead::FREE_SPACE - 4, 0x56);
    df->write_block(data,bb);
    //测试free功能
    //df->free_block(bb);
    //测试read功能
    //vector<uint8_t> result;
    //df->read_block(bb,result);
    delete df;
    delete op;
}

int test_vfs_crerate_seg(int id){
    Env *env = new LinuxEnv();
    Options *op = new Options(env);
    DataFile *df;
    DataFile::open_datafile("myFile",op,&df);
    Segment *sh;
    Segment::create_seg(id,df,op,&sh);//创建这个段
    delete sh;
}

Segment *test_vfs_open_seg(int id){
    Env *env = new LinuxEnv();
    Options *op = new Options(env);
    DataFile *df;
    DataFile::open_datafile("myFile",op,&df);
    Segment *sh;
    Segment::open_seg(id,df,op,&sh);
    return sh;
}

int test_vfs_append(Segment *sh){
    uint32_t offset;
    uint8_t r = (uint8_t)rand();//随机一个输入数据
    cout << r <<endl;
    vector<uint8_t> data{r,0x55,0x56,0x57,0x20,0x20,0x21,0x59};
    sh->append(offset,data);
}

int test_vfs_read(Segment *sh){
    uint32_t offset = 14;
    vector<uint8_t> data;
    sh->read_page(offset,data);
    return 3;
}

int test_vfs_free_page(Segment *sh){
    sh->free_page(14);
}

int test_vfs_write(Segment *sh){
    vector<uint8_t> data{0x57,0x57,0x57,0x57,0x57,0x57,0x57,0x57,0x57,0x57,0x57,0x57,0x57};
    sh->write_page(14,data);
}

int test_vfs_free_seg(Segment *sh){
    sh->free_seg();
}

int main(){
    create_adatafile();
    test_vfs_crerate_seg(1);
    Segment *sh = test_vfs_open_seg(1);
    for(int i = 0;i<20;i++) test_vfs_append(sh);
    test_vfs_read(sh);
    test_vfs_free_page(sh);
    test_vfs_append(sh);
    test_vfs_free_page(sh);
    test_vfs_write(sh);
    test_vfs_free_seg(sh);

    test_vfs_crerate_seg(2);
    test_vfs_crerate_seg(3);
    Segment *sh2 = test_vfs_open_seg(2);
    test_vfs_append(sh2);
    test_vfs_append(sh2);
    test_vfs_write(sh2);




    
    exit(1);
}



/*

*/