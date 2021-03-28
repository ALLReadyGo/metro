#include "metro/net/inner/Timer.h"
#include "metro/net/EventLoop.h"
#include "metro/net/Channel.h"
#include "metro/net/inner/TimerQueue.h"
#include "metro/utils/Logger.h"
#include <sys/timerfd.h>
#include <cassert>
#include <unistd.h>

namespace metro
{

static int creatTimerfd()
{
    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if(timerfd < 0)
    {
        LOG_ERROR << "timerfd creat error\n";
    }
    return timerfd;
}

static timespec howMuchTimeFromNow(const TimerPoint &when)
{
    auto microsecond = std::chrono::duration_cast<std::chrono::microseconds>(
                            when - std::chrono::steady_clock::now()
                       ).count();
    
    if(microsecond < 100)                                                  // 这里做了个优化，避免间距过短造成的频繁调度
        microsecond = 100;
    
    timespec sp;
    sp.tv_sec = static_cast<time_t>(microsecond / 1000000);
    sp.tv_nsec = static_cast<long>((microsecond % 1000000) * 1000);
    return sp;
}

static void resetTimerfd(int timerfd, const TimerPoint &expiration)
{
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof(newValue));
    memset(&oldValue, 0, sizeof(oldValue));
    newValue.it_value = howMuchTimeFromNow(expiration);                 // 获取下次激活时间，这里获得的是距离下次激活的时间差
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret)
    {
        LOG_SYSERR << "timerfd_settime()\n";
    }
}

static void readTimerfd(int timerfd)
{
    uint64_t howmany;
    ssize_t n = read(timerfd, &howmany, sizeof(howmany));
    if(n != sizeof(howmany))
    {
        LOG_FATAL << "read timer fd error\n";
        abort();
    }
}



TimerQueue::TimerQueue(EventLoop *loop)
  : loop_(loop),
    timerfd_(creatTimerfd())
{   
    timerfdChannelPtr_ = std::make_shared<Channel>(loop, timerfd_);
    timerfdChannelPtr_->setReadCallBack(std::bind(&TimerQueue::handleRead, this));
    timerfdChannelPtr_->enableReading();
}

TimerQueue::~TimerQueue()
{
    
}

/*
 * 
*/
TimerId TimerQueue::addTimer(const TimerCallback &callback, 
                    const TimerPoint &when,
                    const TimerInterval &interval)
{
    TimerPtr timer_ptr = std::make_shared<Timer>(
                callback, when, interval);
    loop_->runInLoop([this, timer_ptr](){                   // 这里使用异步队列调用，addTimer是线程安全的
        addTimerInLoop(timer_ptr);
    });
    return timer_ptr->id();
}   

TimerId TimerQueue::addTimer(const TimerCallback &&callback, 
                    const TimerPoint &when,
                    const TimerInterval &interval)
{
    TimerPtr timer_ptr = std::make_shared<Timer>(
                std::move(callback), when, interval);
    loop_->runInLoop([this, timer_ptr](){
        addTimerInLoop(timer_ptr);
    });
    return timer_ptr->id();
}   

bool TimerQueue::insert(const TimerPtr &timerPtr)
{
    bool flesh_timerfd = false;                                     // 是否需要重新设置定时时间
    if(timers_.empty() || timerPtr->when() < timers_.top()->when())
    {   
        flesh_timerfd = true;
    }
    timers_.push(timerPtr);
    return flesh_timerfd;
}

void TimerQueue::addTimerInLoop(const TimerPtr &timerPtr)
{
    if(timerPtr->when() < std::chrono::steady_clock::now())            // 插入时已经超期，就直接执行
    {
        timerPtr->run();
    }
    assert(timerIdSet_.find(timerPtr->id()) == timerIdSet_.end());
    timerIdSet_.insert(timerPtr->id());                                // timerIdset_保存了所有的可用的TimerID
    if(insert(timerPtr))                                               // insert()函数会在需要更新timerQueue最早时间时返回true
    {   
        resetTimerfd(timerfd_, timerPtr->when());
    }
}

/* 取消之前设置的定时只会从timerIdSet_中移除相应的id，而不会对queue内部元素进行删除，这样的设置能够保证程序的高效执行*/
void TimerQueue::invalidTimer(TimerId id)
{
    loop_->runInLoop([this, id]() { timerIdSet_.erase(id); });          
}

void TimerQueue::reset(const std::vector<TimerPtr> &expired, const TimerPoint &now)
{
    for(auto &it : expired)
    {
        TimerId id = it->id();
        if(timerIdSet_.find(id) != timerIdSet_.end())
        {
            if(it->isRepeat())                              // repeate的话自动设置下次调用时间点
            {
                it->restart(now);
                insert(it);
            }
            else
            {
                timerIdSet_.erase(id);                      // 不是的话，从列表中删除
            }
        }
    }
    if(!timerIdSet_.empty())
    {
        TimerPoint next_expired = timers_.top()->when();    // 获取下次最新的调度时间点
        resetTimerfd(timerfd_, next_expired);
    }
}

std::vector<TimerPtr> TimerQueue::getExpired(const TimerPoint &now)
{
    std::vector<TimerPtr> expired_timer;
    while(!timers_.empty())
    {
        TimerPtr timer = timers_.top();
        if(timer->when() > now)
            break;
        expired_timer.push_back(timer);
        timers_.pop();
    }
    return expired_timer;
}

// Timerfd 的 callback回调事件
void TimerQueue::handleRead()
{
    auto now = std::chrono::steady_clock::now();
    std::vector<TimerPtr> expired_timer = getExpired(now);  // 获取所有过期的Timer
    for(auto &it : expired_timer)
    {
        TimerId id = it->id();
        if(timerIdSet_.find(id) != timerIdSet_.end())       // 这里的判断是为了配合invalid操作，保证只调用那些仍然可用的Timer
        {
            it->run();                                      // 执行Timer的callback
        }
    }
    reset(expired_timer, now);                              // 重置这些Timer
}

}