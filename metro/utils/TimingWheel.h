#include <memory>
#include <unordered_set>
#include <deque>
#include <functional>
#include "metro/net/EventLoop.h"
#include "metro/net/inner/Timer.h"

#define TIMING_BUCKET_NUM_PER_WHEEL 100
#define TIMING_TICK_INTERVAL 1.0

namespace metro
{

using EntryPtr = std::shared_ptr<void>;
using EntryBucket = std::unordered_set<EntryPtr>;
using BucketQueue = std::deque<EntryBucket>;

class TimingWheel
{
  public:
    class CallbackEntry
    {
      public:
        CallbackEntry(std::function<void()> cb)
          : cb_(std::move(cb))
        {
        }
        ~CallbackEntry()
        {
            cb_();
        }
      private:
        std::function<void()> cb_;
    };

    TimingWheel(EventLoop *loop, 
                size_t maxTimeout,
                float ticksInterval = TIMING_TICK_INTERVAL,
                size_t bucketsNumPeerWheel = TIMING_BUCKET_NUM_PER_WHEEL);

    void insertEntry(double delay, EntryPtr entryPtr);

    void insertEntryInLoop(size_t delay, EntryPtr entryPtr);

    EventLoop *getLoop()
    {
        return loop_;
    }
    
  private:
    std::vector<BucketQueue> wheels_;
    std::atomic<size_t> ticksCount_{0};
    EventLoop *loop_;
    TimerId timerId_;

    float ticksInterval_;
    size_t wheelsNum_;
    size_t bucketsNumPerWheel_;
};




}