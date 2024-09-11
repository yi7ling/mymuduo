#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

// IO多路复用模块，抽象类（因为有纯虚函数）
class Poller : noncopyable
{
public:
    typedef std::vector<Channel*> ChannelList;

    Poller(EventLoop* loop_);
    virtual ~Poller() = default;

    // 轮询IO事件，必须被loop线程调用
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0; // 纯虚函数
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // 判断 channel是否在当前 poller中
    virtual bool hasChannel(Channel* channel) const;

    // eventloop通过该接口获取默认的IO复用对象
    static Poller* newDefaultPoller(EventLoop *loop);
protected:
    typedef std::unordered_map<int, Channel*> ChannelMap;
    ChannelMap channels_;
private:
    EventLoop* ownerloop_;// 定义poller所属的eventloop
};