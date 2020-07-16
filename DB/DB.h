#ifndef OB_DB_IMPL
#define OB_DB_IMPL
#include<vector>
#include<string>
#include "../include/status.h"
using namespace std;
class DB{
public:
    DB();
    static Status open(const string &name);//根据指定名字打开一个数据库
    Status create_table(const string &nanme);//创建表
    Status drop_table(const string &name);//删除表
    Status insert_data(const string &name, int col, const string &content);//在表中第几个字段中插入数据
    

private:
};



#endif //.