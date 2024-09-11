#pragma once

#include "noncopyable.h"
#include "functional"
#include "vector"
#include "atomic"
#include "Timestamp.h"
#include "CurrentThread.h"

#include <sched.h>
#include <memory>
#include <mutex>
#include <functional>

class Channel;
class Poller;

// 事件循环类 主要包含两个模块 Channel Poller(epoller抽象类)
class EventLoop : noncopyable
{
public:
    typedef std::function<void()> Functor;

    EventLoop();
    ~EventLoop();

    void loop();

    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 在当前loop中执行 cb
    void runInLoop(Functor cb);
    // 把 cb 放入队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    // 内部使用
    void wakeup(); // 唤醒loop所在的线程

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
private:
    void handleRead(); // wake up
    void doPendingFunctors(); // 执行待办的回调

    typedef std::vector<Channel*> ChannelList;

    std::atomic_bool looping_;          // 原子操作，通过CAS实现
    std::atomic_bool quit_;             // 标识退出loop循环
    std::atomic_bool
        callingPendingFunctors_;        // 标识当前loop是否正在执行的回调操作
    const pid_t threadId_;              // 记录当前loop 所在的线程id
    Timestamp pollReturnTime_;          // poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;

    /**
     * 当mainLoop获取一个新用户的channel，通过轮询算法选择一个subloop，
     * 通过该成员唤醒subloop 处理 channel
     */
    int wakeupFd_; // TODO 是唤醒谁？ 唤醒相应的loop(sub Rector)
    std::unique_ptr<Channel> wakeupChannel_; // 不会把这个channel暴露给客户

    ChannelList activeChannels_; // 发生事件的channel

    std::mutex mutex_; // 互斥锁，用来保护 pendingFunctors_ 线程安全操作
    std::vector<Functor> pendingFunctors_; // 存储subloop需要执行的所有回调操作
};
