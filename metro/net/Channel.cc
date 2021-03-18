#include "metro/net/Channel.h"
#include "metro/net/EventLoop.h"
#include "assert.h"
namespace metro
{

Channel::Channel(EventLoop *loop, int fd)
  : loop_(loop), fd_(fd)
{
}

void Channel::remove()
{
    assert(kNoneEvent == events_);
    addedInLoop_ = false;
    loop_->removeChannel(this);
}
    
void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::handleEvent()
{   
    if(tied_)
    {
        auto guard = tie_.lock();
        if(guard)
        {
            handleEventFree();
        }
    }
    else
    {
        handleEventFree();
    }
}

void Channel::handleEventFree()
{
    if(eventCallBack_)
    {
        eventCallBack_();
    }
    if((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        if(closeCallBack_)
            closeCallBack_();
    }
    if(revents_ & (POLLNVAL | POLLERR))
    {
        if(errorCallBack_)
            errorCallBack_();
    }
    if(revents_ & (POLLIN | POLLPRI | POLLRDHUP))
    {
        if(readCallBack_)
            readCallBack_();
    }
    if(revents_ & (POLLOUT))
    {
        if(writeCallBack_)
            writeCallBack_();
    }
}

}