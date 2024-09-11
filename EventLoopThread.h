#pragma once

#include "noncopyable.h"
#include "Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

class EventLoop;

class EventLoopThread : noncopyable
{
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                  const std::string& name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop();
private:
    void threadFunc();

    EventLoop* loop_; // use mutex
    bool exiting_;
    Thread thread_;
    std::mutex mutex_; // 提供loop_使用，因为存在多个线程的访问
    std::condition_variable cond_; // 做线程间的通信
    ThreadInitCallback callback_;
};
