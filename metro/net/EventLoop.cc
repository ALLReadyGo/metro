#include "metro/net/EventLoop.h"
#include <sys/eventfd.h>
#include "metro/utils/Logger.h"
#include <cassert>
#include <unistd.h>
namespace metro
{

static int creatEventFd()
{
    int event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(event_fd < 0)
    {
        LOG_ERROR << "creat Event fd error\n";
    }
    return event_fd;
}

constexpr int kPollTimeMs = 10000;

thread_local EventLoop *tloopInThisThread = nullptr;

EventLoop::EventLoop()
  : threadId_(std::this_thread::get_id()),
    poller_(Poller::newPoller(this)),
    wakeupFd_(creatEventFd()),
    threadLocalLoopPtr_(&tloopInThisThread),
    timerQueue_(new TimerQueue(this))
{
    tloopInThisThread = this;
    wakeupChannelPtr_ = std::make_unique<Channel>(this, wakeupFd_);
    wakeupChannelPtr_->setReadCallBack(std::bind(&EventLoop::wakeupFd_, this));
    wakeupChannelPtr_->enableReading();
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;

    while(!quit_)
    {
        activedChannels_.clear();
        poller_->poll(kPollTimeMs, activedChannels_);

        eventHandling_ = true;
        for(auto channl : activedChannels_)
        {
            currentChannel = channl;
            currentChannel->handleEvent();
        }
        currentChannel = nullptr;
        eventHandling_ = false;
        doRunInLoopFuncs();                     // 运行func_中等待事件
    }
    looping_ = false;
}

void EventLoop::doRunInLoopFuncs()
{
    while(!func_.empty())
    {
        Func fun;
        func_.dequeue(fun);
        fun();
    }
}

EventLoop::~EventLoop()
{
    assert(!isRunning());
}

void EventLoop::assertInLoopThread()
{
    assert(std::this_thread::get_id() == threadId_);
}

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL << "It is forbidden to run loop on threads other than event-loop "
                 "thread";
    exit(1);
}

void EventLoop::quit()
{
    quit_ = true;

    if (!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::queueInLoop(const Func &f)
{
    func_.enqueue(f);
    if(!isInLoopThread() || !looping_)
    {
        wakeup();
    }
}

void EventLoop::queueInLoop(Func &&f)
{
    func_.enqueue(std::move(f));
    if(!isInLoopThread() || !looping_)
    {
        wakeup();
    }
}

void EventLoop::runInLoop(const Func &f) 
{
    if(isInLoopThread())
    {
        f();
    }
    else
    {
        queueInLoop(f);
    }
}

void EventLoop::runInLoop(Func &&f)
{
    if(isInLoopThread())
    {
        f();
    }
    else
    {
        queueInLoop(std::move(f));
    }
}

bool EventLoop::isInLoopThread()
{
    return std::this_thread::get_id() == threadId_;
}

TimerId EventLoop::runAt(const Date &time, const Func &cb)
{
    auto micro_seconds = 
        time.microSecondsSinceEpoch() - Date::now().microSecondsSinceEpoch();
    std::chrono::steady_clock::time_point tp =
        std::chrono::steady_clock::now() + 
        std::chrono::microseconds(micro_seconds);
    return timerQueue_->addTimer(cb, tp, std::chrono::microseconds(0));
}

TimerId EventLoop::runAt(const Date &time, Func &&cb)
{
    auto micro_seconds = 
        time.microSecondsSinceEpoch() - Date::now().microSecondsSinceEpoch();
    std::chrono::steady_clock::time_point tp =
        std::chrono::steady_clock::now() + 
        std::chrono::microseconds(micro_seconds);
    return timerQueue_->addTimer(std::move(cb), tp, std::chrono::microseconds(0));
}

TimerId EventLoop::runAfter(double delay, const Func &cb)
{
    return runAt(Date::now().after(delay), cb);
}

TimerId EventLoop::runAfter(double delay, Func &&cb)
{
    return runAt(Date::now().after(delay), std::move(cb));
}

TimerId EventLoop::runEvery(double interval, const Func &cb)
{
    std::chrono::microseconds dur = 
        std::chrono::microseconds(static_cast<uint64_t>(interval * 1000000));
    std::chrono::steady_clock::time_point tp = 
        std::chrono::steady_clock::now() + dur;
    return timerQueue_->addTimer(cb, tp, dur);
}

TimerId EventLoop::runEvery(double interval, Func &&cb)
{
    std::chrono::microseconds dur = 
        std::chrono::microseconds(static_cast<uint64_t>(interval * 1000000));
    std::chrono::steady_clock::time_point tp = 
        std::chrono::steady_clock::now() + dur;
    return timerQueue_->addTimer(std::move(cb), tp, dur);
}

void EventLoop::invalidateTimer(TimerId id)
{
    if(isRunning() && timerQueue_)
        timerQueue_->invalidTimer(id);
}

void EventLoop::resetTimeQueue()
{
    assertInLoopThread();
    assert(!looping_);
    timerQueue_.reset();
}

void EventLoop::resetAferFork()
{
    poller_->resetAfterFork();
}

static EventLoop* getEventLoopOfCurrentThread()
{
    return tloopInThisThread;
}

void EventLoop::moveToCurrentThread()
{
    if(isRunning())
    {
        LOG_FATAL << "Eventloop can't be moved when running\n";
        exit(-1);
    }
    if(isInLoopThread())
    {
        LOG_WARN << "This EventLoop already in the current thread\n";
        return;
    }
    if(tloopInThisThread)
    {
        LOG_FATAL << "There is already an EventLoop\n";
        exit(-1);
    }
    *threadLocalLoopPtr_ = nullptr;
    tloopInThisThread = this;
    threadLocalLoopPtr_ = &tloopInThisThread;
    threadId_ = std::this_thread::get_id();
}

void EventLoop::updateChannel(Channel *channel)
{
    assert(channel->owerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    assert(channel->owerLoop() == this);
    assertInLoopThread();
    poller_->removeChannel(channel);
}

void EventLoop::wakeup()
{
    uint64_t tmp = 1;
    if(!looping_)
        return;
    write(wakeupFd_, &tmp, sizeof(tmp));
}

void EventLoop::wakeupRead()
{
    ssize_t ret = 0;
    uint64_t tmp;
    ret = read(wakeupFd_, &tmp, sizeof(tmp));
    if(ret < 0)
    {
        LOG_FATAL << "wake up error";
    }
}
}


    

    
