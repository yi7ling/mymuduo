#pragma once

#include "Poller.h"

#include <vector>
#include <sys/epoll.h>

/**
 * 封装epoll的主要功能(api): 
 *      epoll_create
 *      epoll_ctl
 *      epoll_wait
 */
class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    // 封装 epoll_wait
    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;
private:
    static const int kInitEventListSize = 16;

    static const char* operationToString(int op);

    void fillActiveChannels(int numEvents,
                        ChannelList* activeChannels) const;

    // 封装 epoll_ctl
    void update(int operation, Channel* channel);

    // = typedef std::vector<epoll_event> EventList;
    using EventList = std::vector<epoll_event>;

    int epollfd_; // epoll事件自身fd，用于update event
    EventList events_;
};