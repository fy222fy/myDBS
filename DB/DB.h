#ifndef OB_DB_IMPL
#define OB_DB_IMPL
#include<vector>
#include<string>
#include<functional>
#include "../include/status.h"
#include "../include/LOB_locator.h"
#include "../LOB/LOB.h"
#include "../Util/Util.h"
#include "../SQL/SQL.h"

using namespace std;
class DB{
public:
    DB(Options *op){vfs = VFS::get_VFS(op);}//打开数据库
    ~DB(){}
    void running();
    Status create_table(const string &nanme);//创建表
    Status drop_table(const string &name);//删除表
    Status insert(const string &name, uint32_t id, LOBLocator *ll);
    Status select(const string &name, uint32_t id, LOBLocator *ll);
    Status update(const string &name, uint32_t id, LOBLocator *ll);
    Status del(const string &name, uint32_t id);
    //创建一个空的LOcator
    Status create_locator(const string &name, LOBLocator *llp);
    void close();//安全地退出数据库
private:
    VFS *vfs;//数据库用这个操作vfs
    SQL sql;//sql引擎
    hash<string> hashT;//一个hash函数，用于计算seg——id
};

class Table{//只提供顺序查找的表结构
public:
    Table(const string &name,uint32_t t_seg);
    Table(const string &name,uint32_t t_seg,uint32_t l_seg);
    ~Table();
    Status write_record(uint32_t id, LOBLocator *loc);
    Status read_record(uint32_t id, LOBLocator *loc);
    Status update_record(uint32_t id, LOBLocator *loc);
    Status delete_record(uint32_t id);
    Status read_table_head();//从表中读表头,段里的第一个页
    Status write_table_head();//将表头序列化后写入文件
    const uint64_t HEAD_SIZE = 12;
private:
    string name;//表的名字
    uint64_t nums;//记录数量
    uint32_t table_seg_id;
    uint32_t lob_seg_id;
    bool if_lob_table;
};

#endif //.