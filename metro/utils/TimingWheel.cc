#include "metro/utils/TimingWheel.h"


namespace metro
{
TimingWheel::TimingWheel(EventLoop *loop,                                       
                         size_t maxTimeout,                                     // TimingWheel允许的最大超时
                         float ticksInterval,                                   // 每次滴答的时间间隔
                         size_t bucketsNumPeerWheel)                            
  : loop_(loop),
    ticksInterval_(ticksInterval),
    bucketsNumPerWheel_(bucketsNumPeerWheel)
{
    size_t maxTicksNum =  static_cast<size_t>(maxTimeout / ticksInterval) + 1;
    while(maxTicksNum > 0)
    {
        wheelsNum_ += 1;
        maxTicksNum /= bucketsNumPeerWheel;
    }
    wheels_.resize(wheelsNum_);
    for(int i = 0; i < wheelsNum_; ++i)
        wheels_[i].resize(bucketsNumPeerWheel);
    /*
     *  将wheelsNum = 2， bucketsNumPerWheel=60
     *  wheels[1] 对应的是分钟， wheels[0]对应的是秒，每当60秒是分钟就会+1
    */
    timerId_ = loop_->runEvery(ticksInterval, [this](){                         // 每次滴答周期都会调用这个函数
        ticksCount_++;                                                          // 总共的滴答次数
        size_t t = ticksCount_;
        size_t pow = 1;
        for(size_t i = 0; i < wheelsNum_; ++i)
        {
            EntryBucket tmp;
            if((t % pow) == 0)
            {
                wheels_[i].front().swap(tmp);                                   // pop_front, push_back操作相同于计时器的转动
                wheels_[i].pop_front();                                         // tmp析构时会调用所有EntryPtr的析构函数，需要注意的是这是个shared_ptr, CallbackEntry的析构会不会被调用还得看他的引用是否被减为0，这也是实现extendLife的基础
                wheels_[i].push_back(EntryBucket());
            }
            pow = pow * bucketsNumPerWheel_;
        }
    });
}

void TimingWheel::insertEntry(double delay, EntryPtr entryPtr)
{
    if(delay < 0)
        return;
    if(!entryPtr)
        return;
    if(loop_->isInLoopThread())
    {
        insertEntryInLoop(delay, entryPtr);
    }
    else            
    {
        loop_->runInLoop([this, delay, entryPtr](){
            insertEntryInLoop(delay, entryPtr);
        });
    }
}

void TimingWheel::insertEntryInLoop(size_t delay, EntryPtr entryPtr)
{
    loop_->assertInLoopThread();
    delay = static_cast<size_t>(delay / ticksInterval_ + 1);
    size_t t = ticksCount_;
    for(size_t i = 0; i < wheelsNum_; ++i)
    {
        if(delay < bucketsNumPerWheel_)
        {
            wheels_[i][delay - 1].insert(entryPtr);
            return;
        }
        if(i < wheelsNum_ - 1)
        {
            // 这里是一个递归调用，实际插入timingwheel的只会在最高的那个时间点插入
            entryPtr = std::make_shared<CallbackEntry>(     // 返回最新的entryPtr
                [this, entryPtr, i, t, delay]() {           // 这里会保存上一个entryPtr
                    wheels_[i][((delay + (t % bucketsNumPerWheel_) - 1)) % bucketsNumPerWheel_].insert(entryPtr);
            });
        }
        else
        {
            wheels_[i][bucketsNumPerWheel_ - 1].insert(entryPtr);
        }

        delay = (delay + (t % bucketsNumPerWheel_) - 1) / bucketsNumPerWheel_;
        t = t / bucketsNumPerWheel_;
    }
}

}

