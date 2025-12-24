/*
 * @file heaptimer.h
 * @brief HeapTimer类
 */ 
#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>
#include "../log/log.h"

// 回调函数包装器。当定时器超时，会调用这个函数（通常是执行 HttpConn::Close）
typedef std::function<void()> TimeoutCallBack;
// 高精度时钟及其时间点
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
    // 结点的唯一标识，通常就是 Socket 的 fd
    int id;
    // 绝对过期时间点
    TimeStamp expires;
    TimeoutCallBack cb;
    // 定义了小根堆的比较逻辑——过期时间越早，优先级越高（在堆顶）
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }
};
class HeapTimer {
public:
    HeapTimer() { heap_.reserve(64); }

    ~HeapTimer() { clear(); }
    
    // 每当客户端发来一次请求（活跃一次），服务器就调用此函数，把它的过期时间往后推
    void adjust(int id, int newExpires);

    // 添加新定时器。如果该 id 已存在，则更新它；如果不存在，则推入堆尾并上浮
    void add(int id, int timeOut, const TimeoutCallBack& cb);

    void doWork(int id);

    void clear();

    // 在 WebServer 的主循环中被调用。它检查堆顶元素，如果堆顶已过期，则执行回调并弹出；循环往复直到堆顶未过期
    void tick();

    void pop();

    // 返回当前距离最近的一个过期时间点还有多少毫秒
    // 主循环的 epoll_wait 的 timeout 参数直接使用这个值。这样既保证了及时处理超时，又避免了主线程无意义的空转
    int GetNextTick();

private:
    // 删除
    void del_(size_t i);
    
    // 上浮
    void siftup_(size_t i);

    // 下沉
    bool siftdown_(size_t index, size_t n);

    // 交换
    void SwapNode_(size_t i, size_t j);

    // 物理存储结构是一个数组，逻辑结构是一棵完全二叉树（小根堆）
    // 堆顶始终是那个“最快要过期”的连接。查询堆顶的时间复杂度是 O(1)
    std::vector<TimerNode> heap_;
    
	// 存储 id (fd) 到其在 heap_ 数组中下标的映射
    // 空间换时间，标准的堆是不支持随机访问的。如果要更新某个指定 fd 的过期时间，通常需要 O(N) 遍历堆。有了这个 map，我们可以 O(1) 找到节点在堆中的位置，从而将更新操作（adjust）的复杂度降为 O(logN)
    std::unordered_map<int, size_t> ref_;
};

#endif //HEAP_TIMER_H
