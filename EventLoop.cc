#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory>

// 防止一个线程创建多个 EventLoop, __thread == thread local，针对每个线程的全局变量
__thread EventLoop* t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000;// 10s

// 创建wakeupfd, 用来notify唤醒subReator处理新的channel
int createEventfd()
{
    /**
     * eventfd 是 Linux 系统中的一个系统调用，
     * 用于创建一个事件文件描述符（event file descriptor），
     * 它提供了一种轻量级的、可用于进程间通信（IPC）的机制
     */
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

    if (evtfd < 0)
    {
        LOG_FAIAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop create %p in thread %d \n",
        this, threadId_);
    
    if (t_loopInThisThread)
    {
        LOG_FAIAL("Another EventLoop %p exists in this thread %d \n",
            t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型，以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));

    /**
     * 每一个eventloop都将监听wakeupChannel的EPOLLIN读事件了
     * enableReading -> channel::update
     */
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;

    // tips: 因为poller、channel使用了智能指针，所以不需要在析构函数中 delete
}

// 开启事件循环
/**
 * Poller 监听到channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
 */
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while (!quit_)
    {
        activeChannels_.clear();
        // 监听两类fd  client的fd、wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);

        for (Channel* channel: activeChannels_)
        {
            channel->handleEvent(pollReturnTime_);
        }

        // 执行当前EventLoop事件循环需要处理的回调操作
        /**
         * 举例子：
         * IO线程 mainloop收到客户端连接，创建一个client_fd，封装成channel.
         * 需要把channel发送给 subpool做监听处理
         * mainloop 事先注册回调cb传给subloop，
         * wakeup subloop后，执行下面的doPendingFunctors()方法，执行mianloop 注册的cb
         */
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

// 退出事件循环
/**
 * 存在两个地方会被调用
 *  1. loop在自己的线程中调用quit
 *  2. 在非loop的线程中，调用loop的quit
 */
void EventLoop::quit()
{
    quit_ = true;

    // TODO 为什么quit()函数 可以在其他线程中执行？
    // 如果这个loop是在其他线程中调用的quit，那么他会去wakeup 这个loop所在的线程
    if (!isInLoopThread())
    {
        wakeup();
    }
}

// 在当前loop中执行 cb
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else // 在非当前loop线程中执行cb，就需要唤醒loop所在线程，执行cb
    {
        queueInLoop(cb);
    }
}

// 把 cb 放入队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    /**
     * callingPendingFunctors_的逻辑待解释：
     *      当前loop正在执行回调，但是loop又有新的回调。
     *      为了让新的回调能在loop()中被执行（doPendingFunctors()），
     *      需要wakeup()来激活poll，从而继续走poll()的while流程
     */
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup(); // 唤醒loop所在线程
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop:handleRead() reads %d bytes instead of 8", n);
    }
}

 // 唤醒loop所在的线程
 /**
  * 具体唤醒流程
  *     1. 向wakeupfd_ 写一个数据
  *     2. wakeupChannel发生了读事件
  *     3. 监听wakeupChannel 的 poller 通过 epoll_wait得到 events
  *     4. 当前的loop线程被唤醒
  */
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);

    if (n != sizeof one)
    {
        LOG_ERROR(
            "EventLoop::handleRead() reads %d bytes instead of 8", n);
    }
}

void EventLoop::updateChannel(Channel* channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor& functor: functors)
    {
        functor(); // 
    }

    callingPendingFunctors_ = false;
}