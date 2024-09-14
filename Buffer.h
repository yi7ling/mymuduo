#pragma once

#include <vector>
#include <stddef.h>
#include <string>
#include <algorithm>

// 网络库底层的缓冲器类型定义

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    {}

    // 获取xx区域的字节数
    size_t readableBytes() const
    { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const
    { return buffer_.size() - writerIndex_; }
    size_t prependableBytes() const
    { return readerIndex_; }

    // 获取缓冲区中 可读数据的起始地址
    const char* peek() const
    { return begin() + readerIndex_; } // 这里调用的是哪个 begin()函数？

    // onMessage    string <- Buffer
    // 从可读区域获取长度为 len 的数据
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            readerIndex_ += len;
        }
        else
        {
            retrieveAll();
        }
    }

    // 获取可读区域中的所有数据
    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len); // 从可读区域读取len字节数据
        retrieve(len);
        return result;
    }

    void ensureWriteableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len); // 扩容
        }
    }

    // 把[data, data+len]内存上的数据，添加到缓冲区的可写区域
    void append(const char* data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }

    char* beginWrite()
    { return begin() + writerIndex_; }

    const char* beginWrite() const
    {
        return begin() + writerIndex_; 
    }

    // 从 fd 上读数据
    ssize_t readFd(int fd, int* savedErrno);
private:
    char* begin()
    {
        /**
         * *it的理解, 迭代器对it.operator*()进行了重写,
         * 获取 buffer_中底层数组首元素
         */
        return &*buffer_.begin();
    }

    // 提供给const方法调用
    const char* begin() const
    { return &*buffer_.begin(); }

    void makeSpace(size_t len)
    {
        if (prependableBytes() + writableBytes() < kCheapPrepend + len)
        {
            buffer_.resize(writableBytes() + len);
        }
        else
        {
            size_t readable = readableBytes();

            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_,
                      begin() + kCheapPrepend);
            
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};