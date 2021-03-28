#include "metro/net/inner/Acceptor.h"
#include <unistd.h>

namespace metro
{
Acceptor::Acceptor(EventLoop *loop, 
             const InetAddress &addr,
             bool reUseAddr,
             bool reUsePort)
    : sock_(Socket::createNonblockingSocketOrDie(addr.getSockAddr()->sa_family)),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)),
      addr_(addr),
      loop_(loop),
      acceptorChannel_(loop, sock_.fd())
{
    sock_.setReuseAddr(reUseAddr);
    sock_.setReusePort(reUsePort);
    sock_.bindAddress(addr);
    acceptorChannel_.setReadCallBack(std::bind(&Acceptor::readCallback, this));
    if (addr_.toPort() == "0")
    {
        addr_ = InetAddress{Socket::getLocalAddr(sock_.fd())};
    }
}
    
Acceptor::~Acceptor()
{
    acceptorChannel_.disableAll();
    acceptorChannel_.remove();
}
    
void Acceptor::listen()
{
    sock_.listen();
    acceptorChannel_.enableReading();                      
}
    
void Acceptor::readCallback()
{
    InetAddress peer;
    int newsock = sock_.accept(&peer);                  // 获取连接的socket
    if (newsock >= 0)
    {
        if (newConnectionCallback_)                     // 新的连接建立成功，调用相应callback
        {
            newConnectionCallback_(newsock, peer);
        }
        else                                           
        {
            ::close(newsock);
        }
    }
    else
    {                                                   
        LOG_SYSERR << "Accpetor::readCallback";
        if (errno == EMFILE)                                                // 文件描述符用尽，并发连接数量过大时需要，此时我们应及时处理掉listen socket中等待的socket，释放系统资源
        {
            ::close(idleFd_);                                               // 释放idleFD_，保证我们有一个fd资源
            idleFd_ = sock_.accept(&peer);                                  // 多线程下并不安全，不过如果秉承one progres one acceptor，那没什么问题
            ::close(idleFd_);                                               // 接收并关闭，这个socket我们已经没能力处理，所以直接关闭，拒绝服务
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);            // 这里是阻塞等待，保证idleFd_必定拿到fd
        }
    }
}

}