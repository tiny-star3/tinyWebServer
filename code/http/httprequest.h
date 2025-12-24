/*
 * @file httprequest.h
 * @brief HttpRequest类
 */ 
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>     
#include <mysql/mysql.h>  //mysql

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

class HttpRequest {
public:
    // 有限状态机状态，支持非阻塞解析。如果数据只来了一半，状态机会停在某个位置，下次数据到了接着解析
    enum PARSE_STATE {
        REQUEST_LINE,	// 解析请求行（例如 GET /index.html HTTP/1.1）
        HEADERS,		// 解析请求头（键值对，直到遇到空行）
        BODY,			// 解析请求体（POST 请求携带的数据）
        FINISH,        	// 解析完成
    };

    // 处理结果，定义了解析的结果状态，用于指导 HttpResponse 该返回 200、400 还是 404
    enum HTTP_CODE {
        NO_REQUEST = 0,			// 请求不完整，需要继续读取请求报文数据
        GET_REQUEST,			// 获得了完整的HTTP请求
        BAD_REQUEST,			// HTTP请求报文有语法错误或请求资源为目录
        NO_RESOURSE,			// 请求资源不存在
        FORBIDDENT_REQUEST,		// 请求资源禁止访问，没有读取权限
        FILE_REQUEST,			// 请求资源可以正常访问
        INTERNAL_ERROR,			// 服务器内部错误
        CLOSED_CONNECTION,
    };
    
    HttpRequest() { Init(); }
    ~HttpRequest() = default;

    // 重置所有成员变量。因为连接可能是 Keep-Alive（长连接），一个 HttpRequest 对象会被多次复用，每次解析新请求前必须初始化
    void Init();
    // 从 Buffer 中读取数据，并在 while 循环中根据 state_ 驱动状态机运行
    bool parse(Buffer& buff);

    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;

    bool IsKeepAlive() const;

    /* 
    todo 
    void HttpConn::ParseFormData() {}
    void HttpConn::ParseJson() {}
    */

private:
    // 使用正则表达式拆解请求行，提取 Method, URL 和 Version
    bool ParseRequestLine_(const std::string& line);
    // 按行读取，利用正则表达式或字符串处理提取键值对
    void ParseHeader_(const std::string& line);
    // 根据 Content-Length 读取指定长度的字节作为 Body，未实现可以修改补充
    void ParseBody_(const std::string& line);

    void ParsePath_();
    void ParsePost_();
    // 对 POST 提交的 application/x-www-form-urlencoded 数据进行解码（处理 %20、+ 等转义字符）
    // 当你在网页上填写表单（如登录、注册）并点击提交时，浏览器通常会把数据以 application/x-www-form-urlencoded 格式发送。其格式类似于：username=mark&password=123&email=123%40qq.com
    void ParseFromUrlencoded_();
	
    // 连接数据库的桥梁。它会调用 SqlConnPool 里的连接，执行 SQL 语句（查询或插入），实现真正的用户登录校验或注册入库功能
    static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin);

    PARSE_STATE state_;
    // 存储 HTTP 请求的四个基本组成部分
    std::string method_, path_, version_, body_;
    // 存储请求头的所有键值对（如 Connection: keep-alive）
    std::unordered_map<std::string, std::string> header_;
    // 存储 POST 请求解析出来的表单数据
    std::unordered_map<std::string, std::string> post_;

    // 静态常量，定义了项目中哪些页面是合法的，以及登录/注册对应的特定逻辑
    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static int ConverHex(char ch);
};


#endif //HTTP_REQUEST_H
