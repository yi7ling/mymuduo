#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

// channel在poller中的状态
const int kNew = -1;        // 未加入channels_(map)
const int kAdded = 1;       // 加入channels_，已绑定epoll
const int kDeleted = 2;     // 加入channels_，未绑定epoll

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC)) // 创建一个新的fd并返回
    , events_(kInitEventListSize)
{
    if (epollfd_ < 0) {
        LOG_FAIAL("epoll_create1 error:%d", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    LOG_DEBUG("%s ==> fd total cnt:%u \n",
        __func__, channels_.size());

    int numEvents = ::epoll_wait(epollfd_,
                                 &*events_.begin(),// 取vecotr类型数组的元素首地址
                                 static_cast<int>(events_.size()),
                                 timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {
        fillActiveChannels(numEvents, activeChannels);

        // 扩容检查
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("%s ==> timeout!");
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("EPollPoller::poll");
    }

    return now;
}

void EPollPoller::fillActiveChannels(int numEvents,
                    ChannelList* activeChannels) const
{
    for (int i = 0; i< numEvents; i++)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
        int fd = channel->fd();
        auto it = channels_.find(fd);

        // TODO 为什么 assert(channels_[fd] == channel); 会出错？
        assert(it != channels_.end());
        assert(it->second == channel);
#endif
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

/**
 * 触发流程：channel update/remove ==> eventloop update/remove ==> poller update/remove
 * 
 * 作用：在epollfd上，更新一个fd的事件监听
 * 
 * Poller模块所处的位置：
 *         EventLoop
 *  ChannelList        Poller
 *                  ChannelMap <fd, channel*>
 *                      epollfd_
 */
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();

    LOG_INFO("func:%s ==> fd=%d, events=%d, index=%d \n",
        __func__, channel->events(), index);
    if (index == kNew || index == kDeleted)
    {
        int fd = channel->fd();

        if (index == kNew)
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        else // index == kDeleted
        {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else // channel已经在poller中注册过
    {
        int fd = channel->fd();
        (void)fd;// 表明后续代码不打算使用这个变量，使代码更清晰
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(channel->index() == kAdded);

        if (channel->isNoneEvent())
        {
            channel->set_index(kDeleted);
            update(EPOLL_CTL_DEL, channel);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    int index = channel->index();

    LOG_INFO("func:%s ==> fd=%d, events=%d, index=%d \n",
        __func__, channel->events(), index);

    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());
    assert(index == kAdded || index == kDeleted);

    channels_.erase(fd);
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EPollPoller::update(int operation, Channel* channel)
{
    // int __epfd, int __op, int __fd, epoll_event *
    struct epoll_event event;
    int fd = channel->fd();

    memset(&event, 0, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    if (epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl op=%s, fd=%d \n", operationToString(operation), fd);
        }
        else // EPOLL_CTL_ADD / EPOLL_CTL_MOD
        {
            LOG_FAIAL("epoll_ctl op=%s, fd=%d \n", operationToString(operation), fd);
        }
    }
}

const char* EPollPoller::operationToString(int op)
{
    switch (op)
    {
    case EPOLL_CTL_ADD:
        return "ADD";
        break;
    case EPOLL_CTL_DEL:
        return "DEL";
        break;
    case EPOLL_CTL_MOD:
        return "MOD";
        break;
    default:
        return "Unkown Operation";
    }
}
