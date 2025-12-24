/*
 * @file httpresponse.h
 * @brief HttpResponse类
 */ 
#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    // 初始化与清理
    // 初始化响应对象。注意，由于对象会被复用，每次响应前都要重置文件指针、状态码和路径
    void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
    // 调用 munmap 释放内存映射资源，并重置 mmFile_ 指针。防止内存泄漏的关键
    void UnmapFile();
    
    // 构建响应
    void MakeResponse(Buffer& buff);
    
    char* File();
    size_t FileLen() const;
    
    // 当请求出错（如 404）时，构造一个简单的 HTML 报错页面并存入 Buffer
    void ErrorContent(Buffer& buff, std::string message);
    
    int Code() const { return code_; }

private:
    // 内部填充函数
    // 向 Buffer 写入 HTTP/1.1 200 OK\r\n
    void AddStateLine_(Buffer &buff);
    // 向 Buffer 写入 Content-Length、Content-Type 和 Connection 等信息
    void AddHeader_(Buffer &buff);
    // 通过 open 打开文件，获取文件描述符。
    // 使用 mmap 系统调用。这允许内核直接将磁盘文件映射到用户空间地址，发送时配合 writev 可以极大减少 CPU 拷贝开销
    void AddContent_(Buffer &buff);

    // 当请求出错（如 404）时，构造一个简单的 HTML 报错页面并存入 Buffer
    void ErrorHtml_();
    
    // 根据请求文件的后缀名，确定该文件对应的 HTTP MIME 类型（Content-Type）
    std::string GetFileType_();

    // 响应状态与路径
    // HTTP 状态码（如 200, 404, 403）
    int code_;
    // 是否保持长连接。这决定了响应头中 Connection 字段是 keep-alive 还是 close
    bool isKeepAlive_;
    // 请求的具体资源文件名和服务器的根目录
    std::string path_;
    std::string srcDir_;
    
    // 文件映射
    // 指向由 mmap 映射到内存中的文件起始地址
    char* mmFile_;
    // 存储文件的元信息（通过 stat 系统调用获取），最重要的信息是 文件大小 (st_size)，用于设置 Content-Length
    struct stat mmFileStat_;

    // 静态配置表
    // 映射表（后缀 -> MIME类型）。如 .html -> text/html，.jpg -> image/jpeg
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    // 映射表（状态码 -> 状态描述）。如 200 -> OK
    static const std::unordered_map<int, std::string> CODE_STATUS;
    // 映射表（错误码 -> 错误页面路径）。如 404 -> /404.html
    static const std::unordered_map<int, std::string> CODE_PATH;
};


#endif //HTTP_RESPONSE_H
