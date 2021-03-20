#pragma once

#include "metro/net/inner/Timer.h"
#include <queue>
#include <memory>
#include <unordered_set>

namespace metro
{

class EventLoop;
class Channel;

using TimerPtr = std::shared_ptr<Timer>;
struct ComparerTimerPtr
{
    bool operator()(const TimerPtr &lhs, const TimerPtr &rhs)
    {
        return *lhs > *rhs;
    }
};

class TimerQueue : NonCopyable
{
  public:
    explicit TimerQueue(EventLoop *loop);
    ~TimerQueue();

    TimerId addTimer(const TimerCallback &callback, 
                     const TimerPoint &when,
                     const TimerInterval &interval);

    TimerId addTimer(const TimerCallback &&callback, 
                     const TimerPoint &when,
                     const TimerInterval &interval);
    
    void addTimerInLoop(const TimerPtr &timerPtr);
    void invalidTimer(TimerId id);

    void reset();

  private:
    void handleRead();

    EventLoop *loop_;
    int timerfd_;
    std::shared_ptr<Channel> timerfdChannelPtr_;

    std::priority_queue<TimerPtr, std::vector<TimerPtr>, ComparerTimerPtr> timers_;
    bool callingExpiredTimers_ = false;
    bool insert(const TimerPtr &timerPtr);
    void reset(const std::vector<TimerPtr> &expired, const TimerPoint &now);
    std::vector<TimerPtr> getExpired(const TimerPoint &now);

    std::unordered_set<TimerId> timerIdSet_;
};


}