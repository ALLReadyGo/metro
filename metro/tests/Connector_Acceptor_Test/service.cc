#include "metro/net/inner/Acceptor.h"
#include "metro/net/inner/Connector.h"
#include "metro/net/TcpConnection.h"
#include "metro/net/EventLoopThreadPool.h"
#include "metro/net/inner/TcpConnectionImpl.h"
#include "metro/net/EventLoop.h"
#include "metro/utils/MpscQueue.h"
#include <set>
using namespace metro;

std::set<TcpConnectionPtr> connPtrs;
EventLoopThreadPool eventLoopThreadPoll(1);

void startEchoConnect(int fd, const InetAddress& addr)
{
    std::cout << "coonected" << fd << std::endl;
    std::cout << "addr" << addr.toIpPort() << std::endl;

    auto tcp_connect = std::make_shared<TcpConnectionImpl>(
                            eventLoopThreadPoll.getNextLoop(), fd, 
                            InetAddress(Socket::getLocalAddr(fd)),
                            InetAddress(Socket::getPeerAddr(fd)));
    
    tcp_connect->setRecvMsgCallback(
    [](const TcpConnectionPtr & ptr, MsgBuffer * buff)
    {   
        std::string str(buff->peek(), buff->readableBytes());
        buff->retrieveAll();
        std::cout << "receive" << str << std::endl;
        ptr->send(str);
    });

    tcp_connect->setCloseCallback([]
    (const TcpConnectionPtr &ptr){
        std::cout << "close" << std::endl;
        connPtrs.erase(ptr);    
    });
    connPtrs.insert(tcp_connect);
    tcp_connect->connectEstablished();
}

int main(int argc, char const *argv[])
{

    eventLoopThreadPoll.start();
    EventLoop *loop = eventLoopThreadPoll.getNextLoop();
    InetAddress addr(8000);
    Acceptor acceptor(loop, addr);
    acceptor.setNewConnectionCallback(startEchoConnect);
    loop->runInLoop([&](){
        acceptor.listen();
    });
    
    std::string str;
    
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}
