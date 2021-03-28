/**
 * Poller的虚基类，用于对EpollPoller进行抽象，提供这个基类的作用是屏蔽不同操作系统下poll结构的差异，
 * 具体的Poller可以由kQueue、Select等结构实现
 */
#pragma once
#include "metro/net/Channel.h"
#include "metro/utils/NonCopyable.h"
#include <vector>
namespace metro
{

class EventLoop;
using ChannelList = std::vector<Channel *>;
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