#include "metro/net/inner/Timer.h"

namespace metro
{

std::atomic<TimerId> Timer::timersCreated =  ATOMIC_VAR_INIT(0);

Timer::Timer(const TimerCallback &callback, 
             const TimerPoint &when,
             const TimerInterval &interval)
    : callback_(callback),
      when_(when), 
      interval_(interval),
      id_(++timersCreated),
      repeat_(interval != std::chrono::microseconds(0))
{
}

Timer::Timer(const TimerCallback &&callback, 
             const TimerPoint &when,
             const TimerInterval &interval)
    : callback_(std::move(callback)),
      when_(when), 
      interval_(interval),
      id_(++timersCreated),
      repeat_(interval != std::chrono::microseconds(0))
{
}

void Timer::run()
{
    callback_();
}   

void Timer::restart(const TimerPoint &now)
{
    if(repeat_)
    {
        when_ = now + interval_;
    }
    else
    {
        when_ = std::chrono::steady_clock::now();
    }
}

}