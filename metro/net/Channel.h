/*
 * Channel： IO调度块，提供了面向Eventloop和Channel使用者的两部分调用接口
 * 1、面向Eventloop
 *    handleEvent()
 *      : 当Channel从EventLoop中因为read、write、error事件激活时，EventLoop会对channel执行此函数，自动执行相应的回调函数
 *        这些回调函数都被保存在Channel的内部成员的各种Callback当中
 * 2、面向Channel使用者
 *      update()
 *      ：向EventLoop中注册想要监听的事件，其也可以进行事件取消
 *  
 */
#pragma once
#include <memory>
#include <functional>
#include "metro/utils/NonCopyable.h"

#include <poll.h>
namespace metro
{

class EpollPoller;
class EventLoop;

class Channel : public NonCopyable
{
  public:

    static constexpr int kNoneEvent = 0;
    static constexpr int kReadEvent = POLLIN | POLLPRI;
    static constexpr int kWriteEvent = POLLOUT;
    
    using EventCallBack = std::function<void()>;

    Channel(EventLoop *loop, int fd);

    void setReadCallBack(const EventCallBack &cb)
    {
        readCallBack_ = cb;
    }

    void setReadCallBack(EventCallBack &&cb)
    {
        readCallBack_ = std::move(cb);
    }

    
    void setWriteCallBack(const EventCallBack &cb)
    {
        writeCallBack_ = cb;
    }

    void setWriteCallBack(EventCallBack &&cb)
    {
        writeCallBack_ = std::move(cb);
    }

    void setErrorCallBack(const EventCallBack &cb)
    {
        errorCallBack_ = cb;
    }

    void setErrorCallBack(EventCallBack &&cb)
    {
        errorCallBack_ = std::move(cb);
    }

    
    void setEventCallBack(const EventCallBack &cb)
    {
        eventCallBack_ = cb;
    }

    void setEventCallBack(EventCallBack &&cb)
    {
        eventCallBack_ = std::move(cb);
    }

    void setCloseCallBack(const EventCallBack &cb)
    {
        closeCallBack_ = cb;
    }

    void setCloseCallBack(EventCallBack &&cb)
    {
        closeCallBack_ = std::move(cb);
    }


    int fd() const
    {
        return fd_;
    }

    int events() const
    {
        return events_;
    }    
    
    int revents() const 
    {
        return revents_;
    }

    EventLoop *owerLoop()
    {
        return loop_;
    }

    void remove();

    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }

    bool isNoneEvent() const
    {
        return events_ == kNoneEvent;
    }


    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }

    void disableReading()
    {
        events_ |= ~kReadEvent;
        update();
    }

    bool isReading() const
    {
        return kReadEvent & events_;
    }

    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }

    void disableWriting()
    {
        events_ |= ~kWriteEvent;
        update();
    }

    bool isWriting() const
    {
        return kWriteEvent & events_;
    }

    void updateEvents(int events)
    {
        events_ = events;
        update();
    }

    void tie(const std::shared_ptr<void> tie)
    {
        tie_ = tie;
        tied_ = true;
    }

  private:

    friend class EpollPoller;
    friend class EventLoop;
    void update();
    void handleEvent();
    void handleEventFree();

    int setRevent(int revt)
    {
        revents_ = revt;
        return revt;
    }

    int index()
    {
        return index_;
    }

    void setIndex(int index)
    {
        index_ = index;
    }

    EventLoop *loop_;                       // Channel 所绑定的EventLoop
    const int fd_;                          // Channel绑定的文件描述符
    int events_ = kNoneEvent;               // Channel设置的监听事件
    int revents_ = kNoneEvent;              // Channel执行完poll操作之后返回的激活事件
    int index_ = 0;                         // 用于标记channel是否被加入epoll当中
    
    bool addedInLoop_{false};

    /* 各种Callback */
    EventCallBack readCallBack_;            
    EventCallBack writeCallBack_;
    EventCallBack errorCallBack_;
    EventCallBack closeCallBack_;
    EventCallBack eventCallBack_;
    
    std::weak_ptr<void> tie_;
    bool tied_{false};
};

}