#pragma once
/**
 * 实现的异步安全队列，原作者实现的是一个lock free结构，我这里设计简单，只是利用了mutex进行了简单实现
 */
#include <mutex>
#include <list>
template<typename T>
class MpscQueue
{
  public:
    MpscQueue()
    {
    }

    ~MpscQueue()
    {
    }

    void enqueue(const T &item) 
    {
        std::lock_guard<std::mutex> guard(mtx_);
        items_.push_back(item);
    }

    void enqueue(T &&item)
    {
        std::lock_guard<std::mutex> guard(mtx_);
        items_.push_back(std::move(item));
    }

    bool dequeue(T &item)
    {
        std::lock_guard<std::mutex> guard(mtx_);
        if(items_.empty())
            return false;
        item = items_.front();
        items_.pop_front();
        return true;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> guard(mtx_);
        return items_.empty();
    }

  private:
    mutable std::mutex mtx_;
    std::list<T> items_;
};
