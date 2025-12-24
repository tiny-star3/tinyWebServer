/**
 * @file buffer.h
 * @brief Buffer类
 */

#ifndef BUFFER_H
#define BUFFER_H
#include <cstring>   //perror
#include <iostream>
#include <unistd.h>  // write
#include <sys/uio.h> //readv
#include <vector> //readv
#include <atomic>
#include <assert.h>
class Buffer {
public:
    Buffer(int initBuffSize = 1024);
    ~Buffer() = default;
    
    // 容量与大小相关
    // 返回当前缓冲区末尾还有多少空位可以写
    size_t WritableBytes() const;
    // 返回当前有多少字节的数据还没被处理（即有效数据长度）
    size_t ReadableBytes() const ;
    // 返回 readPos_ 前面的空间大小。当有效数据向后移动或被读取后，前面的空间可以被回收利用
    size_t PrependableBytes() const;

    // 位置定位
    // 返回指向有效数据起始位置的指针（类似于 readPos_ 的地址），但不移动指针
    const char* Peek() const;
    // 返回指向可写区起始位置的指针（即 writePos_ 的地址）
    const char* BeginWriteConst() const;
    char* BeginWrite();
    
    // 写入操作 (Append)
    // 核心函数。在写入数据前调用，检查末尾空闲空间是否够 len。如果不够，会调用 MakeSpace_ 进行扩容或挪动数据
    void EnsureWriteable(size_t len);
    // 手动移动 writePos_ 指针。当你直接向 BeginWrite() 拷贝数据后，需调用此函数告知 Buffer 写入了多少
    void HasWritten(size_t len);
	// 一系列重载函数，用于将字符串、原始内存数据或另一个 Buffer 的数据追加到当前 Buffer 中
    void Append(const std::string& str);
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buff);
    
    // 读取与回收操作 (Retrieve)
    // 读完了 len 长度的数据，将 readPos_ 后移
    void Retrieve(size_t len);
    // 读到某个特定位置（通常配合 std::search 查找 \r\n 使用）
    void RetrieveUntil(const char* end);
	// 清空缓冲区，重置指针
    void RetrieveAll() ;
    // 将所有有效数据转为字符串返回，并清空缓冲区
    std::string RetrieveAllToStr();
	
    // 文件描述符交互 (IO)
    // 极其重要。从 Socket 读取数据。它内部使用了 readv（分散读）技术：如果 Buffer 空间不够，它会利用一个临时的栈空间来接收多出的数据，然后再追加到 Buffer 中。这保证了即使缓冲区小，也能一次性读完 Socket 里的数据
    ssize_t ReadFd(int fd, int* Errno);
    // 将 Buffer 中的有效数据写入 Socket
    ssize_t WriteFd(int fd, int* Errno);

private:
    // 返回 Buffer 内存块的起始地址（底层 vector 的首地址）
    char* BeginPtr_();
    const char* BeginPtr_() const;
    // 内部扩容机制
    void MakeSpace_(size_t len);
	
    // 实际存储的数据
    std::vector<char> buffer_;
    // 读写位置下标
    // 将 readPos_ 和 writePos_ 定义为 std::atomic 意义不大，一个缓冲区在同一时间内只会被一个线程操作（读取或写入），那么在这个线程处理期间，readPos_ 和 writePos_ 是不存在多线程竞争
    std::atomic<std::size_t> readPos_;
    std::atomic<std::size_t> writePos_;
};

#endif //BUFFER_H
