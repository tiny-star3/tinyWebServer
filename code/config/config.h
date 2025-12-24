#ifndef CONFIG_H
#define CONFIG_H

#include "../server/webserver.h"

using namespace std;

class Config
{
public:
    Config();
    ~Config(){};

    void parse_arg(int argc, char*argv[]);
    
    // 端口号
    int port_;
    
    // 触发组合模式
    int trigMode_;
    
    // 超时时间，单位是毫秒ms
    int timeoutMS_;
    
    // 优雅关闭链接
    bool OptLinger_;
    
    // Mysql 端口号
    int sqlPort_;
    
    // Mysql 账号
    const char* sqlUser_;
    
    // Mysql 密码
    const char* sqlPwd_;
    
    // Mysql 数据库名
    const char* dbName_;
    
    // 数据库连接池数量
    int sqlNum_;
    
    // 线程池数量
    int threadNum_;
    
    // 日志开关
    bool openLog_;
    
    // 日志等级
    int logLevel_;
    
    // 日志队列大小，大于0为异步，小于等于0为同步
    int logQueSize_;

};

#endif
