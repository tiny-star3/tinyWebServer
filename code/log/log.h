/*
 * @file log.h
 * @brief Log类
 */ 
#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>           // vastart va_end
#include <assert.h>
#include <sys/stat.h>         //mkdir
#include "blockqueue.h"
#include "../buffer/buffer.h"

class Log {
public:
    // 初始化日志系统。决定是异步还是同步（取决于 maxQueueSize 是否大于 0），并创建后台线程
    void init(int level, const char* path = "./log", 
                const char* suffix =".log",
                int maxQueueCapacity = 1024);

    // 单例模式 (Singleton)，确保整个程序运行期间只有一个 Log 实例，全局共享一个日志文件句柄，避免多个实例同时写文件
    static Log* Instance();
    static void FlushLogThread();

    void write(int level, const char *format,...);
    void flush();

    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() { return isOpen_; }
    
private:
    // 单例模式 (Singleton)，确保整个程序运行期间只有一个 Log 实例，全局共享一个日志文件句柄，避免多个实例同时写文件冲突
    Log();
    
    void AppendLogLevelTitle_(int level);
    virtual ~Log();
    // 后台线程执行的函数，死循环调用 deque_->pop() 并写入 fp_
    void AsyncWrite_();

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    // 文件与路径管理
    // 当前打开的日志文件指针
    FILE* fp_;
    // 日志存放目录（如 ./log）和后缀名（如 .log）
    const char* path_;
    const char* suffix_;
    // 单个日志文件的最大行数（默认 50,000）
    int MAX_LINES_;
    // 记录当前文件的行数和日期。用于实现日志自动翻滚（Rotation）：如果日期变了或者行数满了，就新建一个日志文件
    int lineCount_;
    int toDay_;

    // 标记日志系统当前是否处于运行状态
    bool isOpen_;
 
    // 缓冲区与级别
    // 日志内容暂存区。在格式化字符串（使用 snprintf）时使用
    Buffer buff_;
    // 日志过滤等级（DEBUG, INFO, WARN, ERROR）。只有高于或等于该等级的日志才会被记录
    int level_;
    
    bool isAsync_;

    std::unique_ptr<BlockDeque<std::string>> deque_; 
    std::unique_ptr<std::thread> writeThread_;
    
    // 并发控制
    // 保护 fp_ 和内部计数器（如 lineCount_）的互斥锁。注意，队列 deque_ 内部自带锁
    std::mutex mtx_;
};

// do { ... } while(0)：C++ 宏的经典技巧，确保宏在 if-else 等各种语法结构中能被当成一个独立语句，且必须以分号结尾
// ##__VA_ARGS__：处理变长参数。前面的 ## 可以在参数为空时自动消去逗号，避免编译错误
#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif //LOG_H
