#pragma once

#include "noncopyable.h"

#include <functional>
#include <thread>
#include <memory>
#include <string>
#include <atomic>

class Thread : noncopyable
{
public:
    typedef std::function<void()> ThreadFunc;
    explicit Thread(ThreadFunc, const std::string& name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const { return name_; }

    static int numCreated() { numCreates_; }

private:
    void setDefaultName();

    bool started_;
    bool joined_;
    /**
     * 区别c语言库pthread的使用，这里采用 c++11的 thread库
     * 进行新线程的操作
     */
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;

    static std::atomic_int numCreates_; // 原子类型
};