#pragma once
#include "metro/net/EventLoop.h"
#include <thread>
#include <future>
#include <mutex>
namespace metro
{

class EventLoopThread
{
  public:
    explicit EventLoopThread(const std::string &threadName);
    ~EventLoopThread();

    void run();
    void wait();

    EventLoop *getLoop();

  private:
    EventLoop *loop_;
    std::string loopThreadName_;
    void loopFuncs_();
    std::promise<EventLoop*> promiseForLoopPointer_;
    std::promise<int> promiseForRun_;
    std::promise<int> promiseForLoop_;
    std::once_flag once_;
    std::thread thread_;
};


}