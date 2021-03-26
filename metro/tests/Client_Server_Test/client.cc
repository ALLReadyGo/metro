#include <iostream>
#include "metro/net/TcpClient.h"
#include "metro/net/EventLoopThreadPool.h"
#include "metro/net/TcpConnection.h"
#include "metro/utils/MsgBuffer.h"
#include <future>

using namespace metro;
int main(int argc, char const *argv[])
{
    std::cout << "client" << std::endl;
    
    auto loop_pool_ptr = std::make_shared<EventLoopThreadPool>(1);
    loop_pool_ptr->start();

    std::string echo_str;
    std::cin >> echo_str;
    std::promise<int> echo_compelete;

    TcpClient client(loop_pool_ptr->getNextLoop(), InetAddress("172.21.4.217", 8000), "client");
    client.setConnectionCallback([echo_str](const TcpConnectionPtr & ptr){
        ptr->send(echo_str);
        ptr->shutdown();
    });
    client.setRecvMessageCallback([&echo_compelete](const TcpConnectionPtr &ptr, MsgBuffer * buff){
        std::cout << std::string(buff->peek(), buff->readableBytes());
        echo_compelete.set_value(1);
        ptr->shutdown();
    });
    client.connect();
    echo_compelete.get_future().get();
    for(auto it : loop_pool_ptr->getLoops())
    {
        it->quit();
    }
    loop_pool_ptr->wait();
    return 0;
}
