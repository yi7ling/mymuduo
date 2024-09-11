#pragma once

#include "noncopyable.h"
#include "Timestamp.h" // 包含头文件

#include <functional>
#include <memory>

class EventLoop; // 前向声明，详细解释见 myduo_note.md

/**
 * Channel 理解为通道
 * 封装的内容：
 *          1. sockfd
 *          2. 感兴趣的事件
 *          3. 实际发生的事件，如 EPOLLIN, EPOLLOUT
 *          4. 绑定具体事件处理（回调）函数
 */
class Channel : noncopyable
{
public:
    typedef std::function<void()> EventCallback;
    typedef std::function<void(Timestamp)> ReadEventCallback;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd 得到 poller 通知之后, 统一处理事件函数的入口
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 绑定智能指针，防止当 channel 被手动 remove, channel还在执行回调操作
    // TODO 这个函数用在哪？
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }
    // used by pollers, Poller监听到fd实际发生的事件, 设置给 channel
    int set_revents(int revt) { revents_ = revt; }

    // 设置fd 相应的事件状态
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    // 返回fd 对感兴趣事件的状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    // for poller, 查看channel在epoll中的状态
    int index() { return index_; }

    // 设置channel在epoll中的状态
    void set_index(int idx) { index_ = idx; }

    // one (event)loop per thread
    EventLoop* ownerLoop() { return loop_; }
    void remove();
private:

    void update(); // 更新,public 函数调用
    void handleEventWithGuard(Timestamp receiveTime); // 受保护的处理函数

    static const int kNoneEvent; // 没有任何事件
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;   // 事件循环类,这个channel 归属于那个loop
    const int fd_;      // Poller监听的对象
    int events_;        // 感兴趣事件
    int revents_;       // Poller返回的具体发生的事件
    int index_;         // 用于 Poller

    // 用于跟踪 std::shared_ptr 所管理的对象，但不拥有对象的所有权
    std::weak_ptr<void> tie_;
    bool tied_;

    // 为什么channel 负责调用具体事件处理函数? 因为channel能够知道 fd实际发生了什么事件 revents
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};