#include "DB.h"
#include<functional>
Table::Table(const string &name,uint32_t t_seg)
    :name(name),
    table_seg_id(t_seg){
    
}

Status Table::read_table_head(){

}

Status Table::write_table_head(){
    
}
Status Table::write_record(int id, LOBLocator *loc){

}
Status Table::read_record(int id, LOBLocator *loc){

}
Status Table::update_record(int id, LOBLocator *loc){

}
Status Table::delete_record(int id){
    
}
