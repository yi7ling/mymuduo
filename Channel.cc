#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <poll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_ (-1)
    , tied_(false)
{
}

Channel::~Channel() {}

// channel的tie方法什么时候调用过？ TcpConnection => channel
// 一个TcpConnection新连接创建的时候
void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}

/**
 * 通过eventloop，调用poller相应方法，注册fd的events事件
 * 解释：
 *  当改变channel中的 fd 感兴趣事件时，需要更新 poller 里面的 fd对应事件epoll_ctl
 *  因为channel poller是两个没有从属关系的模块，所以需要通过 eventloop 进行更新
 */
void Channel::update()
{
    loop_->updateChannel(this);
}

// 在 channel所属的 eventloop中，删除该 channel
void Channel::remove()
{
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp timestamp)
{
    std::shared_ptr<void> guard;
    if (tied_) {
        guard = tie_.lock(); // 弱指针升级为强指针
        if (guard) {
            handleEventWithGuard(timestamp);
        }
    } else {
        handleEventWithGuard(timestamp);
    }
}

// 根据poller通知的 fd实际发生的事件，channel 负责调用具体的回调函数
void Channel::handleEventWithGuard(Timestamp timestamp)
{
    LOG_INFO("channel handleEvent revents:%d\n", revents_);

    if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        if (closeCallback_) closeCallback_();
    }

    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (errorCallback_) errorCallback_();
    }

    if (revents_ & (POLLIN | POLLPRI | POLLHUP))
    {
        if (readCallback_) readCallback_(timestamp);
    }

    if (revents_ & POLLOUT)
    {
        if (writeCallback_) writeCallback_();
    }
}
