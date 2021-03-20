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
    const TimerId id_;
    TimerPoint when_;
    TimerInterval interval_;
    bool repeat_;
    static std::atomic<TimerId> timersCreated;
};


}