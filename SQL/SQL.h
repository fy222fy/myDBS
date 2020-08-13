#include<iostream>
#include<string>
#include <vector>
#include <memory>
using namespace std;

#define NOFUNC 999
#define CREATE 1000
#define DROP 1001
#define INSERT 1002
#define SELECT 1003
#define UPDATE 1004
#define DELETE 1005
#define EMPTY 1006

struct ANY{
    int func;
    string name;
    uint32_t ID;
    int loc;//locator
    ANY():func(NOFUNC),name(""),loc(-1),ID(0){}
};

class SQL{
public:
    bool prase(vector<string> &V, ANY &out);
private:

};