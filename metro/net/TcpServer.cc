#include "metro/net/TcpServer.h"
#include "metro/net/inner/TcpConnectionImpl.h"
namespace metro
{
using namespace std::placeholders;

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &address,
                     const std::string &name,
                     bool reUseAddr,
                     bool reUsePort)
  : loop_(loop),
    acceptorPtr_(new Acceptor(loop, address, reUseAddr, reUsePort)),
    serverName_(name)
{
    acceptorPtr_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
}

void TcpServer::newConnection(int fd, const InetAddress &peer)
{
    loop_->assertInLoopThread();
    EventLoop *ioLoop = nullptr;
    if(loopPoolPtr_ && loopPoolPtr_->size() > 0)
    {
        ioLoop = loopPoolPtr_->getNextLoop();
    }
    else
    {
        ioLoop = loop_;
    }
    std::shared_ptr<TcpConnectionImpl> newPtr = std::make_shared<TcpConnectionImpl>(
        ioLoop, fd, InetAddress(Socket::getLocalAddr(fd)), peer);
    if(idleTimeOut_ > 0)
    {
        newPtr->enableKickoffEntry(idleTimeOut_, timingWheelMap_[ioLoop]);
    }

    newPtr->setRecvMsgCallback(recvMessageCallback_);

    newPtr->setConnectionCallback(
        [this](const TcpConnectionPtr &connectionPtr) {
            if (connectionCallback_)
                connectionCallback_(connectionPtr);
        });
    newPtr->setWriteCompleteCallback(
        [this](const TcpConnectionPtr &connectionPtr) {
            if (writeCompleteCallback_)
                writeCompleteCallback_(connectionPtr);
        });
    newPtr->setCloseCallback(std::bind(&TcpServer::connectionClosed, this, _1));
    connSet_.insert(newPtr);
    newPtr->connectEstablished();

}

void TcpServer::start()
{
    std::cout << "starting begin" << std::endl;
    loop_->runInLoop([this]() {
        assert(!started_);
        started_ = true;
        if (idleTimeOut_ > 0)
        {
            timingWheelMap_[loop_] =
                std::make_shared<TimingWheel>(loop_,
                                              idleTimeOut_,
                                              1.0F,
                                              idleTimeOut_ < 500
                                                  ? idleTimeOut_ + 1
                                                  : 100);
            if (loopPoolPtr_)
            {
                auto loopNum = loopPoolPtr_->size();
                while (loopNum > 0)
                {
                    auto poolLoop = loopPoolPtr_->getNextLoop();
                    timingWheelMap_[poolLoop] =
                        std::make_shared<TimingWheel>(poolLoop,
                                                      idleTimeOut_,
                                                      1.0F,
                                                      idleTimeOut_ < 500
                                                          ? idleTimeOut_ + 1
                                                          : 100);
                    --loopNum;
                }
            }
        }
        acceptorPtr_->listen();
        
    });
}

void TcpServer::stop()
{
    loop_->runInLoop([this]() { acceptorPtr_.reset(); });
    for(auto connection : connSet_)
    {
        connection->forceClose();
    }
    loopPoolPtr_.reset();
    for(auto &iter : timingWheelMap_)
    {
        std::promise<int> pro;
        auto f = pro.get_future();
        iter.second->getLoop()->runInLoop([&iter, &pro]() mutable {
            iter.second.reset();
            pro.set_value(1);
        });
        f.get();
    }
}

void TcpServer::connectionClosed(const TcpConnectionPtr &connectionPtr)
{
    loop_->runInLoop([this, connectionPtr]() {
        size_t n = connSet_.erase(connectionPtr);
        (void)n;
        assert(n == 1);
    });

    static_cast<TcpConnectionImpl *>(connectionPtr.get())->connectDestroyed();
}

const std::string TcpServer::ipPort() const
{
    return acceptorPtr_->addr().toIpPort();
}


}