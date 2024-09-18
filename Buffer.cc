#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/**
 * 从fd上读取数据，Poller工作在LT模式
 * 
 * Buffer缓冲区是有大小的，但是从fd_上读数据的时候，却不知道tcp数据最终的大小
 */
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    char extrabuf[65536] = {0}; // 栈上的内存空间 64k
    struct iovec vec[2];
    const size_t writeable = writableBytes();
    vec[0].iov_base = begin() + writeable;
    vec[0].iov_len = writeable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    // 当buffer_中有足够空间时，不会使用到 extrabuf
    // 当 extrabuf 被使用时，最多读取 128k-1 字节
    const int iovcnt = (writeable < sizeof extrabuf) ? 2 : 1;

    // 先把数据读到 buffer_ 和 extrabuf
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *savedErrno = errno; // 返回错误码
    }
    else if (n <= writeable)
    {
        writerIndex_ += n;
    }
    else // writeable < n：extrabuf也写入了数据
    {
        writerIndex_ = buffer_.size();

        // 对buffer_进行扩容，再把n-writeable的数据从 extrabuf转移到 buffer_
        append(extrabuf, n - writeable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int* savedErrno)
{
    // 把 buffer_可读的数据写到 fd中
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *savedErrno = errno;
    }
    return n;// n >= 0
}
