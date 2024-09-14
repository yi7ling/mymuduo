#pragma once

// 对外的服务器编程使用类


#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "Logger.h"

#include <functional>
#include <string>
#include <atomic>
#include <unordered_map>

class TcpServer : noncopyable
{
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;

    enum Option
    {
        kNoReusePort,
        kReusePort
    };

    TcpServer(EventLoop* loop,
              const InetAddress &listenAddr,
              const std::string& nameArg,
              Option option = kNoReusePort);
    ~TcpServer();

    // 设置 subloop的数量
    void setThreadNum(int numThreads);

    void setThreadInitCallback(const ThreadInitCallback& cb)
    { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallbck& cb)
    { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }


    // 开启服务器监听
    void start();

private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    typedef std::unordered_map<std::string, TcpConnectionPtr> ConnectionMap;

    EventLoop* loop_; // main loop

    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_; // 运行在mainloop, 主要用于监听新连接
    std::shared_ptr<EventLoopThreadPool> threadPool_; // loop线程池

    // 用户设置的回调
    ConnectionCallback connectionCallback_; // 新连接时的cb
    MessageCallbck messageCallback_;         // 有读写时的cb
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完后的cb

    ThreadInitCallback threadInitCallback_; // loop线程初始化的cb
    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_; // 保存所有的连接
};