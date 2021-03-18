#include "metro/net/inner/Poller.h"
#include "metro/net/inner/Poller/EpollPoller.h"

namespace metro
{

Poller* Poller::newPoller(EventLoop *loop)
{
    // return new 
    return new EpollPoller(loop);
}

}