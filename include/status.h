#ifndef OB_STATUS
#define OB_STATUS
#include<iostream>
#include<string>
class Status{
public:
    Status():code(Success),content(""){}
    ~Status(){}
    //设置为失败,并设置失败消息和错误返回类型
    void SError(const std::string &msg);
    //IO错误
    void IOError(const std::string &msg);
    void FetalError(const std::string &msg);
    //记录不影响程序运行的警告
    void Warning(const std::string &msg);
    //读取错误信息
    std::string err_msg() const {return this->content;}
    //重载=
    void operator=(Status b);
    //判断状态是否为成功状态
    bool isok() const {return code == Success;}

private:  
    std::string content;//错误内容，用于打印
    enum Code{ //可被扩展的返回代码
        Success,
        IOerror,
        Fetalerror,
    };
    Code code;
};

inline void Status::SError(const std::string &msg){
    code = IOerror;
    content = msg;
}

inline void Status::IOError(const std::string &msg){
    code = IOerror;
    content = msg;
}

inline void Status::FetalError(const std::string &msg){
    code = Fetalerror;
    content = msg;
    std::cerr << content << std::endl;
    exit(-1);
}

inline void Status::Warning(const std::string &msg){
    std::cerr << "警告：" + msg << std::endl;
}

inline void Status::operator=(Status b){
    if(this != &b){
        if(!isok()){
            FetalError("遇到多个错误，请先处理上一个错误");
        }
        else{
            code = b.code;
        }
    }
}

#endif //fs