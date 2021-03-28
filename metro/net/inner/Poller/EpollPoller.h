#pragma once
/**
 * EpollPoller实现了对epoll的封装 
 */
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
    /**
     * 通过channel的events属性，来对epoll进行更新，可以进行监听事件的设置，取消
     */
    virtual void updateChannel(Channel *channel) override;
    virtual void removeChannel(Channel *channel) override;
    
  private:
    static constexpr int kInitEventListSize = 16;
    void fillActiveChannels(int numEvents, ChannelList &activeChannels) const;
    void update(int op, Channel *channel);
    int epollfd_;                  // epoll的文件描述符
    EventList events_;             // 存储每次wait_poll操作返回的epoll_event结构
    ChannelMap channels_;          // 保存fd与channel的对应关系，用于Debug
};

}