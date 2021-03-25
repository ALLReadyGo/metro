#include "metro/utils/TimingWheel.h"


namespace metro
{
TimingWheel::TimingWheel(EventLoop *loop, 
                         size_t maxTimeout,
                         float ticksInterval,
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
    timerId_ = loop_->runEvery(ticksInterval, [this](){
        ticksCount_++;
        size_t t = ticksCount_;
        size_t pow = 1;
        for(size_t i = 0; i < wheelsNum_; ++i)
        {
            EntryBucket tmp;
            if((t % pow) == 0)
            {
                wheels_[i].front().swap(tmp);
                wheels_[i].pop_front();
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
            entryPtr = std::make_shared<CallbackEntry>(
                [this, entryPtr, i, t, delay]() {
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

