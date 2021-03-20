#pragma once

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
