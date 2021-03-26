#include <iostream>
#include "metro/net/EventLoopThreadPool.h"
#include "metro/net/TcpServer.h"
#include "metro/net/TcpConnection.h"
#include "metro/utils/MsgBuffer.h"

/*
    echo service 
*/
using namespace metro;
int main(int argc, char const *argv[])
{
    std::cout << "server" << std::endl;
    
    auto acceptor_loop_pool_ptr = std::make_shared<EventLoopThreadPool>(1);
    acceptor_loop_pool_ptr->start();

    TcpServer server(acceptor_loop_pool_ptr->getNextLoop(), InetAddress(8000), "main_server");
    server.setIoLoopNum(1);
    server.setRecvMessageCallback([](const TcpConnectionPtr &ptr, MsgBuffer *buff){
        ptr->send(buff->peek(), buff->readableBytes());
        buff->retrieveAll();
    });

    server.start();
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
