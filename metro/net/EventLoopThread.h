/**
 * 用于启动EventLoop线程，这里涉及到需要promise操作，其目的是为了控制线程执行流程
 *
 */
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

    void run();                 // 创建完成并不会直接运行，而是需要显式调用run，thread才会运行
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