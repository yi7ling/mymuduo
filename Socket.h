#pragma once

#include "noncopyable.h"

class InetAddress;

// 封装socket fd
class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {}

    ~Socket();

    int fd() const { return sockfd_; }

    // 绑定本地地址，告诉操作系统这个套接字应该监听哪个端口和IP地址
    void bindAddress(const InetAddress& localaddr);
    void listen(); // 监听套接字

    /**
     * 接受客户端的连接请求，创建一个新的套接字用于与客户端通信，
     * 并获取客户端的地址信息。
     */
    int accept(InetAddress* peeraddr);

    void shutdownWrite(); // 关闭写端口


    // 更改tcp选项
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};