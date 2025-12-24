/*
 * @file webserver.h
 * @brief WebServer类
 */ 
#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"

class WebServer {
public:
    WebServer(
        int port, int trigMode, int timeoutMS, bool OptLinger, 
        int sqlPort, const char* sqlUser, const  char* sqlPwd, 
        const char* dbName, int connPoolNum, int threadNum,
        bool openLog, int logLevel, int logQueSize);

    ~WebServer();
    
    // 一个死循环，不断调用 epoller_->Wait()。一旦有动集触发，就根据 fd 的类型派发任务
    void Start();

private:
    // 经典四步走：socket() -> setsockopt() -> bind() -> listen()
    bool InitSocket_(); 
    // 根据配置决定是使用 LT（水平触发） 还是 ET（边缘触发）
    void InitEventMode_(int trigMode);
    void AddClient_(int fd, sockaddr_in addr);
  
    // 处理新连接。接受新客户端，封装成 HttpConn 存入 users_，并挂到 epoller_ 和 timer_ 上
    void DealListen_();
    // 当某个连接可读/写时，主线程不自己干活，而是调用 threadpool_->AddTask(...)，把 OnRead_ 或 OnWrite_ 逻辑扔给工作线程
    void DealWrite_(HttpConn* client);
    void DealRead_(HttpConn* client);

    void SendError_(int fd, const char*info);
    void ExtentTime_(HttpConn* client);
    void CloseConn_(HttpConn* client);

    // 调用 HttpConn::read 读取数据，然后进入 OnProcess
    void OnRead_(HttpConn* client);
    // 调用 HttpConn::write 将生成的响应发回给客户端
    void OnWrite_(HttpConn* client);
    // 调用 HttpConn::process 进行逻辑解析（状态机解析）
    void OnProcess(HttpConn* client);

    static const int MAX_FD = 65536;

    static int SetFdNonblock(int fd);

    bool openLinger_;
    int timeoutMS_;  /* 毫秒MS */
    bool isClose_;
    
    // 网络与状态
    // 服务器监听的端口
    int port_;
    // 监听套接字，专门负责接待“新客人”
    int listenFd_;
    // 静态资源的根目录
    char* srcDir_;
    // 存储监听 Socket 和普通连接 Socket 的 epoll 事件配置（ET 还是 LT）
    uint32_t listenEvent_;
    uint32_t connEvent_;
    
    // 核心组件（智能指针管理）
    // 基于小根堆，负责踢掉那些占着连接不干活的超时客户端
    std::unique_ptr<HeapTimer> timer_;
    // 负责处理具体的读写解析任务，实现并发
    std::unique_ptr<ThreadPool> threadpool_;
    // 监控所有 Socket 的动静
    std::unique_ptr<Epoller> epoller_;
    
    // 数据容器
    // 记录了当前所有连接的文件描述符（fd）与其对应的 HttpConn 对象。通过 fd 快速定位是哪个客户端在说话
    std::unordered_map<int, HttpConn> users_;
};


#endif //WEBSERVER_H
