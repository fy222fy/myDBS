#include "DB.h"
Status DB::create_table(const string &name){
    Status s;
    uint32_t s_id = hashT(name);
    vfs->create_seg(s_id);
    uint32_t l_id = hashT(name+"lob");
    vfs->create_seg(l_id);
    Table table(name,s_id,l_id);
    table.write_table_head();
    return s;
}
Status DB::drop_table(const string &name){
    Status s;
    uint32_t s_id = hashT(name);
    uint32_t l_id = hashT(name+"lob");
    if(!vfs->if_have_seg(s_id) || !vfs->if_have_seg(l_id)) s.FetalError("不存在这个表，无法删除");
    vfs->free_seg(s_id);
    vfs->free_seg(l_id);
    return s;
}

Status DB::insert(const string &name, uint32_t id, LOBLocator *ll){
    Status s;
    uint32_t s_id = hashT(name);
    uint32_t l_id = hashT(name+"lob");
    if(!vfs->if_have_seg(s_id) || !vfs->if_have_seg(l_id)) s.FetalError("不存在这个表，无法删除");
    Table table(name,s_id,l_id);
    table.read_table_head();//读出表结构
    table.write_record(id,ll);
    table.write_table_head();
    return s;
}
Status DB::select(const string &name, uint32_t id, LOBLocator *ll){
    Status s;
    uint32_t s_id = hashT(name);
    uint32_t l_id = hashT(name+"lob");
    if(!vfs->if_have_seg(s_id) || !vfs->if_have_seg(l_id)) s.FetalError("不存在这个表，无法读取");
    Table table(name,s_id,l_id);
    table.read_table_head();//读出表结构
    table.read_record(id,ll);
    return s;
}
Status DB::update(const string &name, uint32_t id, LOBLocator *ll){
    Status s;
    uint32_t s_id = hashT(name);
    uint32_t l_id = hashT(name+"lob");
    if(!vfs->if_have_seg(s_id) || !vfs->if_have_seg(l_id)) s.FetalError("不存在这个表，无法删除");
    Table table(name,s_id,l_id);
    table.read_table_head();//读出表结构
    table.update_record(id,ll);
    table.write_table_head();
    return s;
}
Status DB::del(const string &name, uint32_t id){
    Status s;
    uint32_t s_id = hashT(name);
    uint32_t l_id = hashT(name+"lob");
    if(!vfs->if_have_seg(s_id) || !vfs->if_have_seg(l_id)) s.FetalError("不存在这个表，无法删除");
    Table table(name,s_id,l_id);
    table.read_table_head();//读出表结构
    table.delete_record(id);
    table.write_table_head();
    return s;
}

Status DB::create_locator(const string &name, LOBLocator *llp){
    Status s;
    uint32_t l_id = hashT(name+"lob");//找到lob的seg_id
    LOB lob;
    lob.create_locator(llp,l_id);
    return s;
}

void DB::close(){
    ;
}

void DB::running(){
    while(true){
        vector<string> V;//保存所有操作
        while (true){
            string temp;
            cin >> temp;
            if(temp.find_first_of(';') != string::npos){
                V.emplace_back(temp.substr(0,temp.find_first_of(';')));
                break;
            }
            else V.emplace_back(temp);
        }
        
    }
}