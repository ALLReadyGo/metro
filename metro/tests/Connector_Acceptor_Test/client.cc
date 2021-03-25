#include "metro/net/inner/Acceptor.h"
#include "metro/net/inner/Connector.h"
#include "metro/net/TcpConnection.h"
#include "metro/net/inner/TcpConnectionImpl.h"
#include "metro/net/EventLoopThreadPool.h"
#include "metro/net/EventLoop.h"
#include <future>
using namespace metro;


int main(int argc, char const *argv[])
{
    std::promise<int> conn_fd_promise;
    std::promise<std::string> echo_str;

    EventLoopThreadPool eventLoopThreadPoll(1);
    eventLoopThreadPoll.start();
    EventLoop *loop = eventLoopThreadPoll.getNextLoop();
    InetAddress addr("172.21.4.217", 8000);
    auto conn_ptr = std::make_shared<Connector>(loop, addr);
    conn_ptr->setNewConnectionCallback([&conn_fd_promise](int fd){
        std::cout << "connected  " << fd << std::endl;
        conn_fd_promise.set_value(fd);
    });
    conn_ptr->start();
    int conn_fd = conn_fd_promise.get_future().get();
    auto tcp_connect = std::make_shared<TcpConnectionImpl>(loop, conn_fd, InetAddress(Socket::getLocalAddr(conn_fd)), InetAddress(Socket::getPeerAddr(conn_fd)));
    
    tcp_connect->setRecvMsgCallback(
    [&echo_str](const TcpConnectionPtr &ptr, MsgBuffer *buff){
        std::cout << "recv ing" << std::endl;
        auto str = std::string(buff->peek(), buff->readableBytes());
        buff->retrieveAll();
        // echo_str.set_value(); 
        std::cout << str;
    });
    tcp_connect->setWriteCompleteCallback(
        [](const TcpConnectionPtr &ptr){
            std::cout << "write complete call back" << std::endl;
    });

    tcp_connect->connectEstablished();
    std::cout << "ending" << std::endl;
    tcp_connect->sendFile("/home/heng/metro/metro/tests/Connector_Acceptor_Test/file");
    // tcp_connect->send("Hello World\n");

    std::cout << "ending" << std::endl;
    std::cout << echo_str.get_future().get() << std::endl;
    for(auto loop_it : eventLoopThreadPoll.getLoops())
    {
        loop_it->quit();
    }
    eventLoopThreadPoll.wait();
    return 0;
}
