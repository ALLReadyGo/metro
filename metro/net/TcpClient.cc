#include "metro/net/TcpClient.h"
#include "metro/net/inner/TcpConnectionImpl.h"
#include <functional>
#include <mutex>

namespace metro
{
using namespace std::placeholders;

static void defaultConnectionCallback(const TcpConnectionPtr &conn)
{
    ;
}

static void defaultRecvMessageCallback(const TcpConnectionPtr &, MsgBuffer *buf)
{
    buf->retrieveAll();
}

TcpClient::TcpClient(EventLoop *loop,
                     const InetAddress &service,
                     const std::string &name)
  : loop_(loop),
    connector_(new Connector(loop, service)),
    name_(name),
    connectionCallback_(defaultConnectionCallback),
    recvMessageCallback_(defaultRecvMessageCallback),
    retry_(false),
    connect_(true)
{
    connector_->setNewConnectionCallback(
        std::bind(&TcpClient::newConnection, this, _1));
    connector_->setConnetionErrorCallback([this](){
        if(connectionErrorCallback_)
        {
            connectionErrorCallback_();
        }
    });
}
    
TcpClient::~TcpClient()
{
    std::shared_ptr<TcpConnectionImpl> conn;
    {
        std::lock_guard<std::mutex> gurad(mutex_);
        conn = std::dynamic_pointer_cast<TcpConnectionImpl>(connection_);
    }
    if(conn)
    {
        assert(loop_ == conn->getLoop());
        loop_->runInLoop([conn, loop = loop_]{
            conn->setCloseCallback([loop](const TcpConnectionPtr &connPtr){
                loop->queueInLoop([connPtr](){
                    static_cast<TcpConnectionImpl *>(connPtr.get())
                        ->connectDestroyed();
                });
            });
        });
        conn->forceClose();
    }
}

void TcpClient::connect()
{
    connect_ = false;
    connector_->start();
}

void TcpClient::disconnect()
{
    connect_ = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connection_)
        {
            connection_->shutdown();
        }
    }
}


void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(Socket::getPeerAddr(sockfd));
    InetAddress localAddr(Socket::getLocalAddr(sockfd));
    std::shared_ptr<TcpConnectionImpl> conn;
    conn = std::make_shared<TcpConnectionImpl>(loop_,
                                               sockfd,
                                               localAddr,
                                               peerAddr);
    conn->setConnectionCallback(connectionCallback_);
    conn->setRecvMsgCallback(recvMessageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, _1));
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());
    {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop(
        std::bind(&TcpConnectionImpl::connectDestroyed,
                  std::dynamic_pointer_cast<TcpConnectionImpl>(conn)));
    if (retry_ && connect_)
    {
        connector_->restart();
    }
}


}