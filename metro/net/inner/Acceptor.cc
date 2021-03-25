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
    int newsock = sock_.accept(&peer);
    if (newsock >= 0)
    {
        if (newConnectionCallback_)
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
        if (errno == EMFILE)
        {
            ::close(idleFd_);
            idleFd_ = sock_.accept(&peer);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}

}