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
#include "DB/DB.h"
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
    Env *env = new LinuxEnv();
    Options *op = new Options(env);
    VFS *vfs = VFS::get_VFS(op);
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

void construct_data(uint8_t *data){
    uint8_t d = '0';
    uint8_t c = 'A';
    for(int i = 0; i < 10000;i++){
        if(data[i - 1] == '9'){
            d = '0';
        }
        if(data[i - 1] == 'z'){
            data[i] = d++;
            c = 'A';
        }
        else{
            data[i] = c++;
        }
    }
}

int test_lob(){
    Env *env = new LinuxEnv();
    Options *op = new Options(env);
    VFS *vfs = VFS::get_VFS(op);
    LOB BBB;
    uint32_t lob_seg_id;
    BBB.create_lobseg(lob_seg_id);//创建一个lob段
    LOBLocator *ll = new LOBLocator();
    uint8_t data[10000];
    int fd = open("book",O_RDWR);
    read(fd,data,5000);
    uint8_t data2[10000];
    for(int i =0;i<10000;i++) data2[i] = 'B';
    data[50] = 'F';
    uint8_t result[1000];
    int a = LOBLocator::INLINE_MAX_SIZE + LOBimpl::OUTLINE_1_MAX_SIZE + LOBimpl::OUTLINE_2_MAX_SIZE +LOBimpl::OUTLINE_3_MAX_SIZE;
    BBB.WRITEAPPEND(ll,1000,data);
    BBB.FRAGMENT_INSERT(ll,100,500,data2);
    BBB.READ(ll,300,400,result);
    return 0;
}

int test_DB(){
    uint8_t data[10000];
    int fd = open("book",O_RDWR);
    read(fd,data,5000);
    uint8_t data2[10000];
    for(auto &c:data2) c='B';
    uint8_t result[10000];

    Env *env = new LinuxEnv();
    Options *op = new Options(env);
    DB *db = new DB(op);
    //数据库已经打开，之后进行各种操作

    db->create_table("lobTable");
    LOBLocator *l = new LOBLocator();
    LOBLocator *l2 = new LOBLocator();
    LOBLocator *l3 = new LOBLocator();
    db->create_locator("lobTable",l);//创建一个locator
    db->create_locator("lobTable",l2);//创建一个locator
    db->create_locator("lobTable",l3);//创建一个locator
    LOB lob;
    lob.WRITEAPPEND(l3,1000,data);
    db->insert("lobTable",1,l);
    db->insert("lobTable",2,l2);
    db->insert("lobTable",3,l3);
    db->update("lobTable",2,l3);
    LOBLocator *temp = new LOBLocator();
    db->select("lobTable",2,temp);
    //db->del("lobTable",2);
    db->drop_table("lobTable");
    db->create_table("myTable");
    LOBLocator *ll = new LOBLocator();
    LOBLocator *ll2 = new LOBLocator();
    db->create_locator("myTable",ll);//创建一个locator
    db->create_locator("myTable",ll2);//创建一个locator
    db->insert("myTable",1,ll);
    db->insert("myTable",1,ll2);
    LOBLocator *ll3 = new LOBLocator();
    db->select("myTable",1,ll3);
    lob.WRITEAPPEND(ll3,70,data);
    db->update("myTable",1,ll3);
    db->select("myTable",1,ll3);
    lob.READ(ll3,20,0,result);
    lob.ERASE(ll3,10,5);
    lob.FRAGMENT_INSERT(ll3,100,20,data2);
    db->update("myTable",1,ll3);
    return 0;
}

int main(){
    //a_single_fun_to_test_VFS();
    //test_lob();
    test_DB();
    exit(1);
}



/*

*/