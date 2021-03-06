#include <metro/net/inner/Poller/EpollPoller.h>
#include <unistd.h>
#include <cassert>
#include <cstring>
#include <metro/utils/Logger.h>

namespace metro
{

static constexpr int kNew = 0;
static constexpr int kAdded = 1;
static constexpr int kDeleted = 2;

EpollPoller::EpollPoller(EventLoop *loop)
  : Poller(loop), events_(kInitEventListSize)
{
    epollfd_ = epoll_create1(EPOLL_CLOEXEC);
}

EpollPoller::~EpollPoller()
{
    close(epollfd_);
}

void EpollPoller::poll(int timeoutMs, ChannelList &cl)
{
    int num_events = epoll_wait(epollfd_, &events_[0], events_.size(), timeoutMs);
    int save_errno = errno;
    if(num_events > 0)
    {
        fillActiveChannels(num_events, cl);
        if(num_events == events_.size())
        {
            events_.resize(events_.size() * 2);             // 这里有一个内存分配，可以用于提高内存利用效率
        }
    }
    else if(num_events == 0) 
    {
        ;
    } 
    else 
    {
        if(save_errno != EINTR)
        {
            LOG_ERROR << "poll error:" << save_errno;
        }
    }
}

void EpollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    if(index == kNew || index == kDeleted)                          // index = kNew or kDeleted 表示其已经从poll中移除，此时对EPOLL的操作为CTL_ADD
    {
        const int fd = channel->fd();
        if(index == kNew)
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        else
        {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->setIndex(kAdded);
        update(EPOLL_CTL_ADD, channel);                        
        
    }
    else if(index == kAdded)
    {
        const int fd = channel->fd();
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        if(channel->isNoneEvent())                                // events什么都不监听时，会从poll中删除
        {
            channel->setIndex(kDeleted);
            update(EPOLL_CTL_DEL, channel);                         
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);                       // 调用MOD
        }
    }
}

void EpollPoller::removeChannel(Channel *channel)
{
    const int fd = channel->fd();                                 // 这部分代码全部都用于Debug，能够让我们发现在运行时的代码潜在Bug
    const int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);

    auto n = channels_.erase(fd);
    assert(n == 1);
    assert(channel->isNoneEvent());
    if(index == kAdded && channel->isNoneEvent())
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->setIndex(kNew);
}

void EpollPoller::fillActiveChannels(int numEvents, ChannelList &activeChannels) const
{
    for(size_t i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->setRevent(events_[i].events);
        activeChannels.push_back(channel);
    }
}

void EpollPoller::update(int op, Channel *channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    const int fd = channel->fd();
    event.events = channel->events();
    event.data.ptr = channel;
    if(epoll_ctl(epollfd_, op, fd, &event) < 0)            // 调用epoll的epoll_ctl
    {
        LOG_FATAL << "epoll error\n";
    }
}

}

