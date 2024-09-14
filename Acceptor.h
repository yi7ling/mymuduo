#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

class EventLoop;

class Acceptor : noncopyable
{
public:
    using NewConntectionCallback = std::function<void(int sockfd, const InetAddress&)>;
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool request);
    ~Acceptor();

    void setNewConntectionCallbck(const NewConntectionCallback &cb)
    {
        newConntectionCallback_ = cb;
    }

    bool listenning() const { return listenning_; }
    void listen();
private:
    void handleRead();

    EventLoop* loop_; // Acceptor用的就是用户定义的mainloop,也称作 baseloop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConntectionCallback newConntectionCallback_;
    bool listenning_;
};