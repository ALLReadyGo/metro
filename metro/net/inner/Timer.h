/***
 * Timer结构，与TimerQueue结合使用，共同完成定时器功能
 * 
 */

#pragma once

#include <functional>
#include <atomic>
#include <chrono>
#include "metro/utils/NonCopyable.h"
#include "metro/net/callbacks.h"

namespace metro
{
using TimerId = uint64_t;
using TimerPoint = std::chrono::steady_clock::time_point;
using TimerInterval = std::chrono::microseconds;

class Timer : public NonCopyable
{
  public:
    Timer(const TimerCallback &callback, 
          const TimerPoint &when,
          const TimerInterval &interval);
    
    Timer(const TimerCallback &&callback, 
          const TimerPoint &when,
          const TimerInterval &interval);
    
    ~Timer(){};

    void run();
    void restart(const TimerPoint &now);

    const TimerPoint& when() const
    {
        return when_;
    }

    bool isRepeat() const
    {
        return repeat_;
    }

    TimerId id() const
    {
        return id_;
    }
    /* 这两个操作符的设置保证了其在std的sortable*/
    
    bool operator<(const Timer &other) const
    {
        return when_ < other.when_;
    }

    bool operator>(const Timer &other) const
    {
        return when_ > other.when_;
    }

  private:
    TimerCallback callback_;
    const TimerId id_;                  //   Timer的id
    TimerPoint when_;                   //   Timer的调用时间点
    TimerInterval interval_;
    bool repeat_;
    /*
     * 这个表示创建了多少个timerid，关于timerid的生成机制，其实就是一个不断自增的数
     * ，因为数值很大，如果timer定时都是那种很短的，正常情况下不会出现timer重复问题，但是临界情况下并不是百分百安全
    */
    static std::atomic<TimerId> timersCreated;          
    
};


}