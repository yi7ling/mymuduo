#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>

/**
 * newDefaultPoller()的实现为什么要单独创建一个文件？
 *  因为newDefaultPoller 的函数实现中要去构建派生类实例，这需要引用派生类（Epoller）的头文件
 *  如果放在Poller.cc中去实现，会出现 基类 引用 派生类 的情况，这种
 *  设计显然不合理，所以单独创建一个文件（公共区域）来实现。
 */
Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if (::getenv("MUDUO_USE_POLL"))
    {
        return nullptr; // 生成poll实例
    }
    else
    {
        return new EPollPoller(loop); // 生成epoll实例
    }
}
