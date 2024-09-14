#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

EventLoopThreadPool::EventLoopThreadPool
    (EventLoop* baseloop, const std::string& nameArg)
    : baseLoop_(baseloop)
    , name_(nameArg)
    , started_(false)
    , numThreads_(0)
    , next_(0)
{

}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // 不用删除 loop, 它是（在线程函数的线程）栈上的变量（创建于void EventLoopThread::threadFunc()）
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
    started_ = true;

    // 创建 numThreads 个 EventLoop线程
    for (int i = 0; i < numThreads_; i++)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t)); // 改用智能指针，自动释放资源
        loops_.push_back(t->startLoop());
    }

    // 整个服务端只有一个线程（运行baseLoop_的线程）时，运行 baseLoop
    if (numThreads_ == 0 && cb)
    {
        cb(baseLoop_);
    }
}

// round-robin baseLoop_默认以轮询的方式分配channel给subloop
EventLoop* EventLoopThreadPool::getNextLoop()
{
    EventLoop* loop = baseLoop_;

    if (!loops_.empty()) // 轮询获取loop
    {
        loop = loops_[next_];
        ++next_;
        if (next_ >= loops_.size())
        {
            next_ = 0;
        }
    }

    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    if (loops_.empty())
    {
        return std::vector<EventLoop*>(1, baseLoop_);
    }
    else
    {
        return loops_;
    }
}
