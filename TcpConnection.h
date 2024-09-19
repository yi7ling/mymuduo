#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

/**
 * TcpServer => Acceptor => 有一个新用户连接，通过acceptor获取到 connfd
 * 
 * => Tcpconnetion 设置回调 => channel =》 poller => Channel的回调操作
 */
class TcpConnection : noncopyable,
                public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop,
                const std::string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddr() const { return localAddr_; }
    const InetAddress& peerAddr() const { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }
    bool disconnected() const { return state_ == kDisconnected; }

    // 发送数据
    // 把数据buf发送给该TCP连接的客户端
    void send(const std::string& message); // c++11
    // void send(const void* message, int len)

    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallbck& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback cb)
    { writeCompleteCallback_ = cb; }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb,
                                  size_t highWaterMark)
    { highWaterMarkCallback_ = cb, highWaterMark_ = highWaterMark; }

    void setCloseCallback(const CloseCallback& cb) // 仅内部使用
    { closeCallback_ = cb; }

    // 连接建立
    void connectEstablished();
    // 关闭连接
    void shutdown();
    // 连接销毁
    void connectDestroyed();

private:
    // tcp连接 的状态
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
    void handleRead(Timestamp timestamp);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* message, size_t len);

    void shutdownInLoop();

    void setState(StateE s) { state_ = s; };
    const char* stateToString() const;

    EventLoop* loop_; // subloop
    std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Channel> channel_;
    std::unique_ptr<Socket> socket_; // TODO：TcpConnection 中 socket的作用是什么？

    const InetAddress localAddr_; // 本地主机的
    const InetAddress peerAddr_;  // 对端的

    // 回调变量，由 TcpServer 传入
    ConnectionCallback connectionCallback_; // 新连接时的cb
    MessageCallbck messageCallback_;         // 有读写时的cb
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完后的cb

    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;

    size_t highWaterMark_; // 保证收发数据的速度平衡
    Buffer inputBuffer_; // 接收数据的缓冲区
    Buffer outputBuffer_; // 发送数据的缓冲区
};