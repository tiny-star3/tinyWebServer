/*
 * @file epoller.h
 * @brief Epoller类
 */ 
#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <vector>
#include <errno.h>

class Epoller {
public:
    explicit Epoller(int maxEvent = 1024);

    ~Epoller();
    
	// 将一个 Socket (fd) 注册到 epoll 监听名单中
    // events 是你关心的事件（如 EPOLLIN 读、EPOLLOUT 写、EPOLLET 边缘触发等）
    bool AddFd(int fd, uint32_t events);
	// 修改已经存在的 fd 的监听事件
    
    bool ModFd(int fd, uint32_t events);
	// 将 fd 从监听名单中移除。当连接关闭（Close）时，必须调用此函数，否则内核会继续监控一个已经失效的描述符
    bool DelFd(int fd);
    
	// 内部封装了 epoll_wait
	//这是整个程序“阻塞”的地方。程序运行到这里会挂起，直到有 Socket 就绪或超时。它返回就绪事件的数量。它会将就绪的事件填充进私有成员 events_ 中
    int Wait(int timeoutMs = -1);
    
	// 用于在 Wait 返回后，通过索引 i 获取第 i 个就绪的 Socket 是哪个、触发了什么事件
    int GetEventFd(size_t i) const;
    uint32_t GetEvents(size_t i) const;
        
private:
    // epoll_create 创建的 epoll 实例的文件描述符（句柄）
    // 内核中 epoll 事件表的入口。后续所有的增加、删除、修改和等待事件的操作，都需要通过这个 epollFd_ 来告知内核
    int epollFd_;
    
    // 用于存放从内核中返回的就绪事件
    // 当 epoll_wait 发现有 Socket 准备好读或写时，它会将这些 Socket 的信息拷贝到这个 events_ 数组中。使用 std::vector 而不是固定长度数组，是为了方便动态调整最大监听数量 
    std::vector<struct epoll_event> events_;    
};

#endif //EPOLLER_H
