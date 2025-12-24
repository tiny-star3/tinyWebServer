/*
 * @file main.cpp
 */ 
#include <unistd.h>
#include "server/webserver.h"
#include "config/config.h"
#include "signal.h"

int main(int argc, char *argv[]) {
    // signal(SIGPIPE, SIG_IGN);
    //命令行解析
    Config config;
    config.parse_arg(argc, argv);

    WebServer server(
        config.port_, config.trigMode_, config.timeoutMS_, config.OptLinger_,
        config.sqlPort_, config.sqlUser_, config.sqlPwd_, config.dbName_,
        config.sqlNum_, config.threadNum_, config.openLog_, config.logLevel_, config.logQueSize_);
    server.Start();
} 
