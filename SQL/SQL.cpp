#include "SQL.h"
bool SQL::prase(vector<string> &V, ANY &out){
    if(V.size() == 0) return false;
    if(V[0] == "create" || V[0] == "CREATE"){
        if(V[1] != "TABLE" && V[1] != "table") return false;
        string table_name = V[2];
        out.func = CREATE;
        out.name = table_name;
    }
    else if(V[0] == "drop" || V[0] == "DROP"){
        if(V[1] != "TABLE" && V[1] != "table") return false;
        string table_name = V[2];
        out.func = DROP;
        out.name = table_name;
    }   
    else if(V[0] == "insert" || V[0] == "INSERT"){
        out.name = V[1];//tablename，不一定有这个table
        if(V[2] != "VALUE" && V[2] != "value") return false;
        out.ID = stoi(V[3]);
        if(V[4] != "empty_lob()") return false;
    }
}