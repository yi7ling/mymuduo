#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>

Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress& localaddr)
{
    int ret = ::bind(sockfd_, localaddr.getSockAddr(), sizeof(sockaddr_in));

    if (ret != 0)
    {
        LOG_FAIAL("bind sockfd:%d fail", sockfd_);
    }
}

void Socket::listen()
{
    int ret = ::listen(sockfd_, SOMAXCONN);
    if (ret < 0)
    {
        LOG_FAIAL("listen sockfd:%d failed", sockfd_);
    }
}

int Socket::accept(InetAddress* peeraddr)
{
    struct sockaddr_in addr;
    socklen_t len;
    memset(&addr, 0, sizeof addr);
    int connfd = ::accept(sockfd_, (sockaddr*)&addr, &len);
    if (connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdownWrite err");
    }
}


// 更改tcp选项
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
            &optval, sizeof optval);
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
            &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
            &optval, sizeof optval);
    if (ret < 0 && on)
    {
        LOG_ERROR("SO_REUSEPORT failed.");
    }
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
            &optval, sizeof optval);
}