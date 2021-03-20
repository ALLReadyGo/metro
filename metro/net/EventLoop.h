#pragma once
#include <vector>
#include "metro/net/callbacks.h"
#include "metro/net/Channel.h"
#include "metro/net/inner/Timer.h"
#include "metro/net/inner/TimerQueue.h"
#include "metro/utils/Date.h"
#include "metro/net/inner/Poller.h"
#include "metro/utils/MpscQueue.h"
#include "queue"
#include <thread>
#include <iostream>

namespace metro
{
using ChannelList = std::vector<Channel*>;

class EventLoop : public NonCopyable
{
  public:
    EventLoop();

    ~EventLoop();

    void loop();

    void quit();  
    
    void assertInLoopThread();

    void resetTimeQueue();

    void resetAferFork();

    bool isInLoopThread();

    static EventLoop* getEventLoopOfCurrentThread();

    void runInLoop(const Func &f);

    void runInLoop(Func &&f);

    void queueInLoop(const Func &f);

    void queueInLoop(Func &&f);

    TimerId runAt(const Date &time, const Func &cb);
    TimerId runAt(const Date &time, Func &&cb);

    TimerId runAfter(double delay, const Func &cb);
    TimerId runAfter(double delay, Func &&cb);

    TimerId runAfter(const std::chrono::duration<long double> &delay, 
                     const Func &cb)
    {
        return runAfter(delay.count(), cb);
    }

    TimerId runAfter(const std::chrono::duration<long double> &delay,
                     const Func &&cb)
    {
        return runAfter(delay.count(), std::move(cb));
    }

    TimerId runEvery(double interval, const Func &cb);
    TimerId runEvery(double interval, Func &&cb);

    TimerId runEvery(const std::chrono::duration<long double> &delay, 
                     const Func &cb)
    {
        return runEvery(delay.count(), cb);
    }

    TimerId runEvery(const std::chrono::duration<long double> &delay,
                     const Func &&cb)
    {
        return runEvery(delay.count(), std::move(cb));
    }
  
    void invalidateTimer(TimerId id);

    void moveToCurrentThread();

    void updateChannel(Channel *channel);

    void removeChannel(Channel *channel);

    size_t index() const
    {
        return index_;
    }

    void setIndex(size_t index)
    {
        index_ = index;
    }

    bool isRunning()
    {
        return looping_ && (!quit_);
    }

    bool isCallingFunction()
    {
        return callingFuncs_;
    }

  private:

    void abortNotInLoopThread();
    void wakeup();
    void wakeupRead();
    
    bool looping_ = false;
    std::thread::id threadId_;
    bool quit_ = false;                                     // 线程安全
    std::unique_ptr<Poller> poller_;
    ChannelList activedChannels_;
    Channel *currentChannel = nullptr;
    
    bool eventHandling_ = false;
    MpscQueue<Func> func_;                                 //TODO: 替换为异步队列
    std::unique_ptr<TimerQueue> timerQueue_;    
    bool callingFuncs_{false};                  
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannelPtr_;     

    void doRunInLoopFuncs();
    size_t index_{std::numeric_limits<size_t>::max()};
    EventLoop **threadLocalLoopPtr_;
    
};

}