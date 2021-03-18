#pragma once
#include "metro/net/Channel.h"
#include "metro/net/EventLoop.h"
#include "metro/utils/NonCopyable.h"
namespace metro
{

class Poller : public NonCopyable
{
  public:
    explicit Poller(EventLoop *loop)
      : ownerLoop_(loop)
    {
    };

    virtual ~Poller()
    {
    };

    virtual void poll(int timeoutMs, ChannelList &cl) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;
    virtual void resetAfterFork()
    {
        /*:TODO*/
    }
    static Poller* newPoller(EventLoop *loop);

  protected:
    EventLoop *ownerLoop_;
};

}