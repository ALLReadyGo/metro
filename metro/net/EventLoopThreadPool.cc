#include "metro/net/EventLoopThreadPool.h"

namespace metro
{

EventLoopThreadPool::EventLoopThreadPool(size_t threadNum, 
                                         const std::string &name)
  : loopIndex_(0)
{
    for(size_t i = 0; i < threadNum; ++i)
    {
        loopThreadVector_.emplace_back(std::make_shared<EventLoopThread>(name));
    }
}

void EventLoopThreadPool::EventLoopThreadPool::start()
{
    for(size_t i = 0; i < loopThreadVector_.size(); ++i)
    {
        loopThreadVector_[i]->run();
    }
}

void EventLoopThreadPool::wait()
{
    for(size_t i = 0; i < loopThreadVector_.size(); ++i)
    {
        loopThreadVector_[i]->wait();
    }
}

size_t EventLoopThreadPool::size()
{
    return loopThreadVector_.size();
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
    if(loopThreadVector_.size() > 0)
    {
        EventLoop *loop = loopThreadVector_[loopIndex_]->getLoop();
        if(++loopIndex_ == loopThreadVector_.size())
            loopIndex_ = 0;
        return loop;
    }
    return nullptr;
}

EventLoop *EventLoopThreadPool::getLoop(size_t id)
{
    if(id < loopThreadVector_.size())
        return loopThreadVector_[id]->getLoop();
    return nullptr;
}

std::vector<EventLoop *> EventLoopThreadPool::getLoops() const
{
    std::vector<EventLoop *> ret(loopThreadVector_.size());
    for(size_t i = 0; i < loopThreadVector_.size(); ++i)
    {
        ret[i] = loopThreadVector_[i]->getLoop();
    }
    return ret;
}



}