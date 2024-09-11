#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h> // c++11 信号量库

std::atomic_int Thread::numCreates_;

Thread::Thread(ThreadFunc func, const std::string& name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        // thread类提供设置分离线程的方法
        // 让线程独立运行,一旦线程被分离，
        // 就无法再与它进行任何形式的同步或通信
        thread_->detach();
    }
}

/**
 * 在执行start()的线程上创建一个新线程tid_，并记录其信息
 */
void Thread::start()
{
    started_ = true;
    sem_t sem; // 信号量机制，保证 tid_能在start()退出前赋值
    sem_init(&sem, false, 0);

    // 创建一个新线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        tid_ = CurrentThread::tid(); // 获取新线程的tid
        sem_post(&sem);
        func_();
    }));

    sem_wait(&sem);
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreates_;

    if (name_.empty()) {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}
