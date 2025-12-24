/**
 * @file httpconn.h
 * @brief HttpConn类
 */ 

#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
    HttpConn();

    ~HttpConn();
	
    // 当一个新连接到来时被调用。它会重置所有状态，初始化缓冲区，并关联 fd_
    void init(int sockFd, const sockaddr_in& addr);
	// 调用底层 readBuff_.ReadFd()，将数据从内核 Socket 读入用户态缓冲区
    ssize_t read(int* saveErrno);
    // 调用 writev。它会尝试将 iov_ 中指向的响应头和文件内容一并发送给客户端
    ssize_t write(int* saveErrno);

    void Close();

    int GetFd() const;

    int GetPort() const;

    const char* GetIP() const;
    
    sockaddr_in GetAddr() const;
    
    // 调用 request_.parse(readBuff_) 解析请求
    // 如果解析不完整，返回 false 继续读
    // 如果解析完成，调用 response_.MakeResponse() 准备要发送的数据
    // 初始化 iov_：设置好响应头和文件的指针及长度，为接下来的 write 做准备
    bool process();

    int ToWriteBytes() { 
        return iov_[0].iov_len + iov_[1].iov_len; 
    }

    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }
	
    // 是否启用 Edge Triggered (边缘触发) 模式。这决定了服务器处理 IO 的行为（是读一次还是读到尽头）
    static bool isET;
    static const char* srcDir;
    // 静态原子变量。记录当前服务器总共有多少个活跃的客户端连接。所有 HttpConn 对象共享这一个计数器
    static std::atomic<int> userCount;
    
private:
    // 连接对应的文件描述符（Socket）。所有的读写操作都通过这个 fd_ 进行
    int fd_;
    // 客户端的地址信息（IP 和 端口）
    struct  sockaddr_in addr_;
	// 标记该连接是否已经关闭
    bool isClose_;
    
    // 散布写(Gather Write)
    int iovCnt_;
    struct iovec iov_[2];
    
    // 读缓冲区。从客户端读入的原始字节流会先存放在这里，等待 HttpRequest 去解析
    Buffer readBuff_;
    // 写缓冲区。存放生成的 HTTP 响应报文头（Header）
    Buffer writeBuff_;
	
    // 负责“解析”。它会从 readBuff_ 中读取数据，利用状态机识别出 Method (GET/POST)、URL、Headers 等
    HttpRequest request_;
    // 负责“生成”。根据 request_ 解析出的结果，去磁盘查找对应的文件（如 index.html），并构建响应报文（状态码 200/404 等）
    HttpResponse response_;
};


#endif //HTTP_CONN_H
