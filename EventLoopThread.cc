#include "EventLoopThread.h"

#include "EventLoop.h"

#include <mutex>

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
                                 const std::string& name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
    , mutex_()
    , cond_()
    , callback_(cb)
{

}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != NULL)
    {
        loop_->quit();
        thread_.join(); // 等待其子线程执行完
    }
}

EventLoop* EventLoopThread::startLoop()
{
    thread_.start(); // 启动底层新线程

    EventLoop* loop = NULL;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == NULL) // 防止虚假的唤醒，wait后会检查 loop_是否为空
        {
            /**
             * wait 函数在被调用时会原子性地释放传入的互斥锁(lock参数)，并使当前线程进入等待状态
             * 
             * 当条件变量的 wait 函数被唤醒（无论是因为 notify_one/notify_all 被调用，还是因为超时或虚假唤醒）时，
             * 它会在内部重新尝试获取互斥锁lock，然后再次检查等待的条件。
             */
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

// 下面这个方法，是单独在新线程里面运行的 one loop per thread
void EventLoopThread::threadFunc()
{
    // 创建一个独立的EventLoop, 和上面的线程是 一一对应的，one loop per thread
    EventLoop loop;

    if (callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop(); // EventLoop loop ==> Poller.poll
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}
