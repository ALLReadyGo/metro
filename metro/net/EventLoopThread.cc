#include "metro/net/EventLoopThread.h"

namespace metro
{

EventLoopThread::EventLoopThread(const std::string &threadName)
  : loopThreadName_(threadName), 
    thread_([this](){
        loopFuncs_();                                           // 线程启动，并运行loopFuncs_
    })
{
    loop_ = promiseForLoopPointer_.get_future().get();          // 等待线程运行，保证loop_设置完成，避免外部引用错误的loop_
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
    std::call_once(once_, [this](){                         // 避免重复run带来问题
        promiseForRun_.set_value(1);
        promiseForLoop_.get_future().get();
    });
}

void EventLoopThread::loopFuncs_()
{
    EventLoop loop;
    promiseForLoopPointer_.set_value(&loop);                // 设置值
    loop.queueInLoop([this](){
        promiseForLoop_.set_value(1);                       // 表示Loop运行
    });
    promiseForRun_.get_future().get();                      // 等待外部调用run指令
    loop.loop();                                            // 调用loop死循环
    loop_ = nullptr;                                        // loop()执行完毕，意味着loop->quit被调用，此时需要清空资源
}


}

