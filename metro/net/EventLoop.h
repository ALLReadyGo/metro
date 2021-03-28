/**
 * EventLoop是核心组件：
 *    核心组件是Poller、TimerQueue、MpscQueue
 * 
 * Poller：
 *    在loop()、updateChannel()中会被处理
 * 
 * TimerQueue：
 *    runAfter和runEvery的具体调用结构，其相关实现需要参照 Timer.h 和 TimerQueue.h两个具体实现。
 * MpscQueue<function<void()>>:
 *    异步安全的callback队列，其他线程的异步事件都需要进入callback才能作用于EventLoop
 *    这样的设计能够保证异步事件的线程安全和高吞吐性
 */
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

    /**
     * 核心函数，是EventLoopThread的循环处理函数
    */
    void loop();

    void quit();  
    
    void assertInLoopThread();

    void resetTimeQueue();

    void resetAferFork();

    bool isInLoopThread();

    static EventLoop* getEventLoopOfCurrentThread();

    /**
     * 这四个函数是MpscQueue<function<void()>>相关操作函数
    */
    void runInLoop(const Func &f);

    void runInLoop(Func &&f);

    void queueInLoop(const Func &f);

    void queueInLoop(Func &&f);

    /**
     * EventLoop定时器相关函数
    */
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
    
    bool looping_ = false;                                  // 标记当前EventLoop是否启动
    std::thread::id threadId_;                              // EventLoop所属的线程，谨记one thread one loop
    bool quit_ = false;                                     // quit标记，这个标记在loop()函数中用到，用于停止loop()函数
    std::unique_ptr<Poller> poller_;                        // poller指针
    ChannelList activedChannels_;                           // 存储从poller中返回的激活channel
    Channel *currentChannel = nullptr;                      // 标记当前调用的currentChannel
    
    bool eventHandling_ = false;                            // 标记是否正在处理eventHanding
    MpscQueue<Func> func_;                                  // 
    std::unique_ptr<TimerQueue> timerQueue_;                // timerQueue
    bool callingFuncs_{false};                  
    int wakeupFd_;                                          // eventfd，其也会被poller监听，当我们向MpscQueue中插入函数是会激活这个fd，来保证poller能及时处理我们插入的函数
    std::unique_ptr<Channel> wakeupChannelPtr_;             // 相应的Channel

    void doRunInLoopFuncs();
    size_t index_{std::numeric_limits<size_t>::max()};      
    EventLoop **threadLocalLoopPtr_;                        // 指向threadLocal 的EventLoop指针，在moveToCurrentThread()函数中会被用到
    
};

}