#include "config.h"

Config::Config(){
    // 端口号，默认为1316
    port_ = 1316;
    
    // 触发组合模式，默认为3，listen 和 conn 都是ET
    trigMode_ = 3;
    
    // 超时时间，单位是毫秒ms，默认为60000
    timeoutMS_ = 60000;
    
    // 优雅关闭链接，默认为false
    OptLinger_ = false;
    
    // Mysql 端口号，默认为3306
    sqlPort_ = 3306;
    
    // Mysql 账号，默认为"root"
    sqlUser_ = "root";
    
    // Mysql 密码，默认为"centos"
    sqlPwd_ = "centos";
    
    // Mysql 数据库名，默认为"tinyWebServer"
    dbName_ = "tinyWebServer";
    
    // 数据库连接池数量，默认为12
    sqlNum_ = 12;
    
    // 线程池数量，默认为6
    threadNum_ = 6;
    
    // 日志开关，默认打开
    openLog_ = true;
    
    // 日志等级，默认为1
    logLevel_ = 1;
    
    // 日志队列大小，大于0为异步，小于等于0为同步，默认为1024
    logQueSize_ = 1024;
}

void Config::parse_arg(int argc, char*argv[]){
    int opt;
    const char *str = "p:m:o:s:t:l:e:q:"; // 包含正确的参数选项字符串，用于参数的解析，带冒号必须有参数
    while ((opt = getopt(argc, argv, str)) != -1)	// 分析命令行参数
    {
        switch (opt)
        {
        case 'p':
        {
            port_ = atoi(optarg);
            break;
        }
        case 'm':
        {
            trigMode_ = atoi(optarg);
            break;
        }
        case 'o':
        {
            OptLinger_ = (atoi(optarg)==1);
            break;
        }
        case 's':
        {
            sqlNum_ = atoi(optarg);
            break;
        }
        case 't':
        {
            threadNum_ = atoi(optarg);
            break;
        }
        case 'l':
        {
            openLog_ = (atoi(optarg)==1);
            break;
        }
        case 'e':
        {
            logLevel_ = atoi(optarg);
            break;
        }
        case 'q':
        {
            logQueSize_ = atoi(optarg);
            break;
        }
        default:
            break;
        }
    }
}
