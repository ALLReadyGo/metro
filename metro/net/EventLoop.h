#pragma once
#include <vector>
#include "metro/net/Channel.h"
#include <iostream>

namespace metro
{
using ChannelList = std::vector<Channel*>;

class EventLoop
{
  public:
    void removeChannel(Channel *channel) 
    {
        std::cout << "called remove Channel" << std::endl;
    }

    void updateChannel(Channel *channel)
    {
        std::cout << "update Channel" << std::endl;
    }
  
  
};

}