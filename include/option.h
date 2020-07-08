#ifndef OPTION_STRUCT
#define OPTION_STRUCT
#include "env.h"
/**
 * 一个可选的辅助结构，一般使用的是封装好的文件结构env
*/
struct Options{
    Env *env;
    Options(Env *en):env(en){}
    ~Options(){delete env;}
};


#endif //