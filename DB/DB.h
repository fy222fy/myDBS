#ifndef OB_DB_IMPL
#define OB_DB_IMPL
#include<vector>
#include<string>
#include "../include/status.h"
#include "../include/LOB_locator.h"
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

class Table{//只提供顺序查找的表结构
public:
    Table(const string &name,uint32_t t_seg);
    Status write_record(int id, LOBLocator *loc);
    Status read_record(int id, LOBLocator *loc);
    Status update_record(int id, LOBLocator *loc);
    Status delete_record(int id);
private:
    string name;//表的名字
    uint64_t nums;//记录数量
    uint32_t table_seg_id;
    uint32_t lob_seg_id;
    Status read_table_head();//从表中读表头
    Status write_table_head();//将表头序列化后写入文件
};

#endif //.