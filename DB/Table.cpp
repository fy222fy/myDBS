#include "DB.h"

Table::Table(const string &name,uint32_t t_seg)
    :name(name),
    table_seg_id(t_seg),
    if_lob_table(false),
    nums(0){
    
}
Table::Table(const string &name,uint32_t t_seg,uint32_t l_seg)
    :name(name),
    table_seg_id(t_seg),
    lob_seg_id(l_seg),
    if_lob_table(true),
    nums(0){   
}
Table::~Table(){
    
}

Status Table::read_table_head(){
    Status s;
    Convert convert;
    uint8_t result[HEAD_SIZE];
    VFS *vfs = VFS::get_VFS();
    vfs->read_page(table_seg_id,0,result);
    int it = 0;
    lob_seg_id = convert.int8_to_int32(result, it);
    it += 4;
    nums = convert.int8_to_int64(result, it);
    it += 8;
    return s;
}

Status Table::write_table_head(){
    Status s;
    Convert convert;
    VFS *vfs = VFS::get_VFS();
    uint8_t result[HEAD_SIZE];
    uint8_t *temp = convert.int32_to_int8(lob_seg_id);
    int it = 0;
    for(int i =0; i<4;i++) result[it++] = temp[i];
    temp = convert.int64_to_int8(nums);
    for(int i = 0; i < 8; i++) result[it++] = temp[i];
    vfs->write_page(table_seg_id,0,result,HEAD_SIZE);
    return s;
}
Status Table::write_record(uint32_t id, LOBLocator *loc){
    Status s;
    Convert convert;
    uint8_t result[8+loc->get_head_size()];
    int it = 0;
    uint8_t *temp = convert.int32_to_int8(loc->get_head_size());
    for(int i =0; i<4;i++) result[it++] = temp[i];
    temp = convert.int32_to_int8(id);
    for(int i =0; i<4;i++) result[it++] = temp[i];
    loc->Serialize(result + it);
    VFS *vfs =VFS::get_VFS();
    uint64_t offset;
    vfs->append(table_seg_id,offset,result,8+loc->get_head_size());
    nums++;//增加数据量的计数器
    return s;
}
Status Table::read_record(uint32_t id, LOBLocator *loc){
    Status s;
    Convert convert;
    uint32_t size,id_t;
    VFS *vfs =VFS::get_VFS();
    uint8_t result[VFS::PAGE_FREE_SPACE];//我也不知道会读出来多少，但最多这么多
    bool state = true;
    int i = 0;
    for(; i < nums; i++){
        int it = 0;
        vfs->read_page(table_seg_id,i+1,result);//i+1是因为第0个块已经被表用了
        size = convert.int8_to_int32(result,it);
        it += 4;
        id_t = convert.int8_to_int32(result,it);
        it+=4;
        if(id_t != id) continue;
        else{//找到id
            loc->Deserialize(result+it);
            state = false;
        }
    }
    if(state) s.FetalError("没有找到对应列！");
    return s;
}
Status Table::update_record(uint32_t id, LOBLocator *loc){
    Status s;
    Convert convert;
    uint32_t size,id_t;
    VFS *vfs =VFS::get_VFS();
    uint8_t result[VFS::PAGE_FREE_SPACE];//我也不知道会读出来多少，但最多这么多
    bool state = true;
    int i = 1;
    for(; i < nums; i++){
        int it = 0;
        vfs->read_page(table_seg_id,i,result);
        size = convert.int8_to_int32(result,it);
        it += 4;
        id_t = convert.int8_to_int32(result,it);
        it+=4;
        if(id_t != id) continue;
        else{//找到id
            loc->Serialize(result+it);
            state = false;
        }
    }
    if(state) s.FetalError("没有找到对应列！");
    return s;
}
Status Table::delete_record(uint32_t id){
    Status s;
    Convert convert;
    uint32_t size,id_t;
    VFS *vfs =VFS::get_VFS();
    uint8_t result[VFS::PAGE_FREE_SPACE];//我也不知道会读出来多少，但最多这么多
    bool state = true;
    int i = 1;
    for(; i < nums; i++){
        int it = 0;
        vfs->read_page(table_seg_id,i,result);
        size = convert.int8_to_int32(result,it);
        it += 4;
        id_t = convert.int8_to_int32(result,it);
        it+=4;
        if(id_t != id) continue;
        else{//找到id
            vfs->free_page(table_seg_id,i);//删除这页就ok了
        }
    }
    if(state) s.FetalError("没有找到对应列！");
    return s;
}