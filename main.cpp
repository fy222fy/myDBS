#include<iostream>
#include<fstream>
#include<string>
#include<cstring>
#include "include/status.h"
#include "include/env.h"
#include "Util/linux_file.h"
#include "DataFile/Block.h"
#include "DataFile/DataFile.h"
#include "VirtualFileSys/VFS.h"
#include "LOB/LOB.h"
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
    uint8_t v[4] = {0x41,0x42,0x43,0x44};
    Status s = RR->Write(20,v,4);
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
    uint8_t v[BlockHead::FREE_SPACE];
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
    DataFile *df = new DataFile();
    DataFile::create_datafile("myFile",op,df);
    delete df;
    delete op;
}

int open_adatafile(){
    Env *env = new LinuxEnv();
    Options *op = new Options(env);
    DataFile *df = new DataFile();
    DataFile::open_datafile("myFile",op,df);
    delete df;
    delete op;
}

int test_alloc_block(){
    Env *env = new LinuxEnv();
    Options *op = new Options(env);
    DataFile *df = new DataFile();
    DataFile::open_datafile("myFile",op,df);
    BlockHandle bb;
    df->alloc_block(bb);
    uint8_t data[BlockHead::FREE_SPACE - 4];
    memset(data,0x56,BlockHead::FREE_SPACE - 4);
    df->write_block(data,BlockHead::FREE_SPACE - 4,bb);
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
    VFS *vfs = VFS::get_VFS(op);
    vfs->create_seg(id);//创建这个段
}

int test_vfs_append(uint32_t id){
    uint64_t offset;
    uint8_t r = (uint8_t)rand();//随机一个输入数据
    cout << r <<endl;
    uint8_t data[8]={r,0x55,0x56,0x57,0x20,0x20,0x21,0x59};
    VFS *vfs = VFS::get_VFS();
    vfs->append(id,offset,data,8);
}

int test_vfs_read(uint32_t id){
    uint32_t offset = 1;
    uint8_t data[BlockHead::FREE_SPACE] = {0};
    VFS *vfs = VFS::get_VFS();
    vfs->read_page(id,offset,data);
    return 3;
}

int test_vfs_free_page(uint32_t id){
    VFS *vfs = VFS::get_VFS();
    vfs->free_page(id,14);
}

int test_vfs_write(uint32_t id){
    uint8_t data[13] = {0x57,0x57,0x57,0x57,0x57,0x57,0x57,0x57,0x57,0x57,0x57,0x57,0x57};
    VFS *vfs = VFS::get_VFS();
    vfs->write_page(id,14,data,13);
}

int test_vfs_free_seg(uint32_t id){
    VFS *vfs = VFS::get_VFS();
    vfs->free_seg(id);
}

void a_single_fun_to_test_VFS(){
    test_vfs_crerate_seg(1);
    for(int i = 0; i < 20; i++) 
        test_vfs_append(1);
    test_vfs_read(1);
    test_vfs_free_page(1);
    test_vfs_write(1);
    test_vfs_append(1);
    test_vfs_free_seg(1);
    test_vfs_crerate_seg(2);
    test_vfs_crerate_seg(1);
    test_vfs_append(1);
    test_vfs_append(2);
}

int test_lob(){
    Env *env = new LinuxEnv();
    Options *op = new Options(env);
    VFS *vfs = VFS::get_VFS(op);
    LOBimpl *lob = new LOBimpl();
    uint32_t lob_seg_id;
    lob->create_lobseg(lob_seg_id);//创建一个lob段
    LOBLocator *ll = new LOBLocator();
    lob->create_locator(ll,lob_seg_id);
    uint8_t data[10000];
    uint8_t c = 'A';
    for(int i = 0; i < 10000;i++){
        if(data[i-1] >= 'z'){
            c = 'A';
        }
        data[i] = c++;
    }
    lob->append(ll,data,3500);
    vector<uint8_t> data2(4000,0x65);
    //lob->append(ll,data2);
    return 0;
}

int main(){
    //a_single_fun_to_test_VFS();
    test_lob();
    exit(1);
}



/*

*/