#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <bits/shared_ptr.h>
#include <unistd.h>

EventLoop* checkSubLoopNotNull(EventLoop* loop)
{
    if (loop == NULL)
    {
        LOG_FAIAL("mainloop is null");
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop,
            const std::string& name,
            int sockfd,
            const InetAddress& localAddr,
            const InetAddress& peerAddr)
    : loop_(checkSubLoopNotNull(loop)),
      name_(name),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64*1024*1024) // 64M
{
    // 给channel设置回调函数
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1)
    );
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this)
    );
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this)
    );
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this)
    );

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d \n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection:dtor[%s] at fd=%d state=%s \n",
        name_.c_str(), channel_->fd(), stateToString());
}

void TcpConnection::send(const std::string& buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInLoop(std::bind(
                &TcpConnection::sendInLoop,
                this,
                buf.c_str(),
                buf.size()
            ));
        }
    }
}

/**
 * 发送数据
 * 应用写的快，而内核写的慢，所以会把数据先写入缓冲区
 * 同时需要调控发送速率（调用水位回调）
 */
void TcpConnection::sendInLoop(const void* data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing!\n");
        return;
    }

    // 表示channel_ 第一次开始写数据，而且输出缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        // 尝试将 len 个字节的数据从 data 指向的缓冲区写入到 fd 指定的文件描述符
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                // 既然能在这把数据全部发送完，就不用给channel设置epollout事件了
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this())
                );
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop\n");
                if (errno == EPIPE || errno == ECONNREFUSED) // RESET
                {
                    faultError = true;
                }
            }
        }
    }

    /**
     * 若TCP缓冲区不能一次性容纳data，剩余的数据需要保存在outputBuffer_中，
     * 然后给channel注册epollout事件，poller发现tcp的发送缓冲区有空间，
     * 会通知相应的socket -> channel，
     * channel调用writeCallback_（即TcpConnection传入的handleWrite）,
     * 直到把 outputBuffer_上的数据，全部发送给tcp缓冲区
     */
    if (!faultError && remaining)
    {
        // outputBuffer_的待发送长度
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMark_)
        {
            loop_->queueInLoop(std::bind(
                highWaterMarkCallback_,
                shared_from_this(),
                oldLen + remaining
            ));
        }
        outputBuffer_.append((char*)data + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting(); // 注册写事件
        }
    }
}

/**
 * 读是相对服务器而言的
 * 当对端客户端有数据到达时，
 * 服务器会监测到EPOLLIN，就会触发该 fd 上的回调
 * fd -> channel -> handleEvent -> handleRead
 * 从而读走对端发来的数据
 */
void TcpConnection::handleRead(Timestamp timestamp)
{
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &saveErrno);

    LOG_INFO("TcpConnection::handleRead, inputBuffer_, readable_len:%u, writable_len:%u \n",
                inputBuffer_.readableBytes(),
                inputBuffer_.writableBytes());

    if (n > 0) // 有数据
    {
        // 已建立连接的用户，有可读事件发生了，调用用户传入的回调操作onMessage
        messageCallback_(shared_from_this(), &inputBuffer_, timestamp);
    }
    else if (n == 0) // 客户端断开
    {
        handleClose();
    }
    else
    {
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead \n");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting()) // 判断是否可写
    {
        int saveErrno = 0;

        // 把 buffer_ 中的数据写入 fd
        // 在这里的fd 是指向一个网络套接字的文件描述符，
        // 数据会被发送到与套接字连接的远程地址（先发送到套接字缓冲区，然后操作系统负责把缓冲区上的数据传输到网络上）。
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) // 全部写入
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    // TODO 为什么使用queueInLoop，不使用可以吗？
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this())
                    );
                }
                if (state_ == kDisconnecting) // 如果正在关闭中
                {
                    shutdownInLoop(); // 为什么可以直接调用 InLoop
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite \n");
        }
    }
    else
    {
        LOG_ERROR("Connection fd=%d is down, no more writing.\n",
                channel_->fd());
    }
}

// 连接建立
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this()); // 跟踪connection对象，保证能正确使用 connection对象提供的回调
    channel_->enableReading(); // 向poller注册读事件

    // 新连接建立，执行回调
    connectionCallback_(shared_from_this());
}

// 连接销毁
void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); // 从poller中移除所有感兴趣事件
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); // 从poller中注销
}

void TcpConnection::handleClose()
{
    LOG_INFO("fd=%d, state=%s", channel_->fd(), stateToString());
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // 执行连接关闭的回调（用户提供的）
    closeCallback_(connPtr); // 关闭连接的回调（TcpServer提供的，TcpServer::removeConnection）
}

// 用户关闭连接 -> shutdown -> showdownInLoop -> shutdownWrite ->
// EPOLLUP -> Poller -> channel::closeCallback_ -> tcpconnection::handleClose
// -> TcpServer::removeConnection
void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting()) // 说明outputBuffer_中的数据都发送完了
    {
        socket_->shutdownWrite(); // 关闭写端
    }
    // 若为发送完，由handleWriting发送完数据后，调用 shutdownInLoop
}

const char* TcpConnection::stateToString() const
{
    switch (state_)
    {
    case kDisconnected:
        return "kDisconnected";
    case kConnecting:
        return "kConnecting";
    case kConnected:
        return "kConnected";
    case kDisconnecting:
        return "kDisconnecting";
    default:
        return "unknow state";
    }
}

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err;

    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err =errno;
    } else {
        err = optval;
    }

    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d",
            name_.c_str(), err);
}