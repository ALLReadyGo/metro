#pragma once

#include <metro/net/inner/Poller.h>
#include <sys/epoll.h>
#include <map>

namespace metro
{

class EpollPoller : public Poller
{
  public:
    using EventList = std::vector<struct epoll_event>;      
    using ChannelMap = std::map<int, Channel *>;

    explicit EpollPoller(EventLoop *loop);
    virtual ~EpollPoller();

    virtual void poll(int timeoutMs, ChannelList &cl) override;    
    virtual void updateChannel(Channel *channel) override;
    virtual void removeChannel(Channel *channel) override;
    
  private:
    static constexpr int kInitEventListSize = 16;

    void fillActiveChannels(int numEvents, ChannelList &activeChannels) const;
    void update(int op, Channel *channel);
    int epollfd_;
    EventList events_;
    ChannelMap channels_;
};

}