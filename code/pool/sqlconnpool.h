/*
 * @file sqlconnpool.h
 * @brief SqlConnPool类
 */ 
#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

class SqlConnPool {
public:
    // 单例模式 (Singleton)
    static SqlConnPool *Instance();

    MYSQL *GetConn();
    void FreeConn(MYSQL * conn);
    int GetFreeConnCount();

    // 服务器启动时执行。根据传入的数据库地址、账号密码，循环调用 mysql_real_connect 创建指定数量（connSize）的连接，并推入 connQue_
    void Init(const char* host, int port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize);
    // 销毁所有连接
    void ClosePool();

private:
    // 单例模式 (Singleton)
    SqlConnPool();
    ~SqlConnPool();

    // 连接池最大容量
    int MAX_CONN_;
    // 记录当前已使用和空闲的连接数
    int useCount_;
    int freeCount_;

    // 已经建立好、随时可用的 MySQL 连接指针
    std::queue<MYSQL *> connQue_;
    // 保证取/放操作是原子性的，防止多个线程拿到同一个连接
    std::mutex mtx_;
    // POSIX 信号量，连接池的“计数器”。初始值等于连接总数（如 10）。每取走一个连接，信号量减 1（P操作）；如果减到 0，后续线程会阻塞等待。每归还一个连接，信号量加 1（V操作），并唤醒等待的线程
    sem_t semId_;
};


#endif // SQLCONNPOOL_H
