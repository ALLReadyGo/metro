#include "metro/net/EventLoopThread.h"

namespace metro
{

EventLoopThread::EventLoopThread(const std::string &threadName)
  : loopThreadName_(threadName), 
    thread_([this](){
        loopFuncs_();
    })
{
    loop_ = promiseForLoopPointer_.get_future().get();
}

EventLoopThread::~EventLoopThread()
{
    
}

void EventLoopThread::wait()
{
    thread_.join();
}

EventLoop *EventLoopThread::getLoop()
{
    return loop_;
}

void EventLoopThread::run()
{
    std::call_once(once_, [this](){
        promiseForRun_.set_value(1);
        promiseForLoop_.get_future().get();
    });
}

void EventLoopThread::loopFuncs_()
{
    EventLoop loop;
    promiseForLoopPointer_.set_value(&loop);
    loop.queueInLoop([this](){
        promiseForLoop_.set_value(1);
    });
    promiseForRun_.get_future().get();
    loop.loop();
    loop_ = nullptr;
}


}

