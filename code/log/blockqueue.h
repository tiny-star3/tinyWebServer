/*
 * @file blockqueue.h
 * @brief BlockDeque类
 */ 
#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>

template<class T>
class BlockDeque {
public:
    explicit BlockDeque(size_t MaxCapacity = 1000);

    ~BlockDeque();

    void clear();

    bool empty();

    bool full();

    // 关闭队列。将 isClose_ 设为 true，并唤醒所有正在等待的线程（让他们醒来发现队列关了，从而优雅退出）
    void Close();

    size_t size();

    size_t capacity();

    T front();

    T back();

    // 生产者操作 (Push)
    void push_back(const T &item);
    void push_front(const T &item);

    // 消费者操作 (Pop)
    bool pop(T &item);
    bool pop(T &item, int timeout);

    // 唤醒消费者去把队列里剩下的数据处理完
    void flush();

private:
    // 底层的容器，负责实际存储数据。选择 deque 是因为它支持在两端进行高效的 O(1) 插入和删除
    std::deque<T> deq_;

    // 队列的最大容量，防止“生产者”产生数据过快导致内存溢出
    size_t capacity_;

    // 互斥锁，保证原子性。任何对 deq_ 的读写操作（push, pop, size等）都必须先加锁，确保同一时刻只有一个线程在操作队列，防止数据竞争（Data Race）
    std::mutex mtx_;

    bool isClose_;

    // 条件变量，实现阻塞挂起与唤醒
    // 当队列为空时，消费者线程在 condConsumer_ 上等待，不再消耗 CPU。一旦生产者放了东西，就唤醒它
    std::condition_variable condConsumer_;
	// 当队列满时，生产者线程在 condProducer_ 上等待。一旦消费者拿走了东西，就唤醒它
    std::condition_variable condProducer_;
};


template<class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity) :capacity_(MaxCapacity) {
    assert(MaxCapacity > 0);
    isClose_ = false;
}

template<class T>
BlockDeque<T>::~BlockDeque() {
    Close();
};

template<class T>
void BlockDeque<T>::Close() {
    {   
        std::lock_guard<std::mutex> locker(mtx_);
        deq_.clear();
        isClose_ = true;
    }
    condProducer_.notify_all();
    condConsumer_.notify_all();
};

template<class T>
void BlockDeque<T>::flush() {
    // 只有一个后台进程（消费者），唤醒一次即可
    condConsumer_.notify_one();
};

template<class T>
void BlockDeque<T>::clear() {
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
}

template<class T>
T BlockDeque<T>::front() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.front();
}

template<class T>
T BlockDeque<T>::back() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
}

template<class T>
size_t BlockDeque<T>::size() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
}

template<class T>
size_t BlockDeque<T>::capacity() {
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

template<class T>
void BlockDeque<T>::push_back(const T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.size() >= capacity_) {
        condProducer_.wait(locker);
        // 唤醒后判断：如果阻塞期间队列被关闭了，立刻停止并退出
        if(isClose_) {
            return;
        }
    }
    deq_.push_back(item);
    condConsumer_.notify_one();
}

template<class T>
void BlockDeque<T>::push_front(const T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.size() >= capacity_) {
        condProducer_.wait(locker);
        // 唤醒后判断：如果阻塞期间队列被关闭了，立刻停止并退出
        if(isClose_) {
            return;
        }
    }
    deq_.push_front(item);
    condConsumer_.notify_one();
}

template<class T>
bool BlockDeque<T>::empty() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
}

template<class T>
bool BlockDeque<T>::full(){
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= capacity_;
}

template<class T>
bool BlockDeque<T>::pop(T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty()){
        condConsumer_.wait(locker);
        if(isClose_){
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

template<class T>
bool BlockDeque<T>::pop(T &item, int timeout) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty()){
        if(condConsumer_.wait_for(locker, std::chrono::seconds(timeout)) 
                == std::cv_status::timeout){
            return false;
        }
        if(isClose_){
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

#endif // BLOCKQUEUE_H
