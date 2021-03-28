#pragma once
/**
 * EventLoopThread 线程池
 */
#include "metro/net/EventLoopThread.h"
#include "metro/utils/NonCopyable.h"

namespace metro
{

class EventLoopThreadPool : public NonCopyable
{
  public:
    EventLoopThreadPool() = delete;

    EventLoopThreadPool(size_t threadNum, 
                        const std::string &name = "EventLoopThreadPool");
    
    void start();

    void wait();

    size_t size();

    EventLoop *getNextLoop();

    EventLoop *getLoop(size_t id);

    std::vector<EventLoop *> getLoops() const;

  private:
    std::vector<std::shared_ptr<EventLoopThread>> loopThreadVector_;
    size_t loopIndex_;                    // 结合getNextLoop使用，用于循环返回eventloop
};

}