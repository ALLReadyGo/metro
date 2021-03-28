#include "metro/net/inner/Connector.h"
#include <assert.h>
#include <unistd.h>

namespace metro
{

Connector::Connector(EventLoop *loop, 
                     const InetAddress &addr, 
                     bool retry)
  : loop_(loop), serverAddr_(addr), retry_(retry)
{
}

Connector::Connector(EventLoop *loop, 
                     InetAddress &&addr, 
                     bool retry)
  : loop_(loop), serverAddr_(std::move(addr)), retry_(retry)
{
}

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop([this]() { startInLoop(); });              // 启动连接
}

void Connector::restart()
{
}

void Connector::stop()
{
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(status_ == Status::Disconnected);
    if(connect_)
    {
        connect();
    }
}

void Connector::connect()                                       // 实际的连接步骤
{
    int sockfd = Socket::createNonblockingSocketOrDie(serverAddr_.family());        // 启动一个非阻塞套接字
    errno = 0;
    int ret = Socket::connect(sockfd, serverAddr_);             // 会立即返回
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:                                           // 如果连接的是回环地址，此时可能会直接连接上
            LOG_TRACE << "connecting";
            connecting(sockfd);                                 // 一切正常
            break;
        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            if (retry_)
            {
                retry(sockfd);
            }
            break;
        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_SYSERR << "connect error in Connector::startInLoop "
                       << savedErrno;
            ::close(sockfd);
            if (errorCallback_)
                errorCallback_();
            break;

        default:
            LOG_SYSERR << "Unexpected error in Connector::startInLoop "
                       << savedErrno;
            ::close(sockfd);
            if (errorCallback_)
                errorCallback_();
            break;
    }
}

void Connector::connecting(int sockfd)
{
    status_ = Status::Connecting;
    assert(!channelPtr_);
    channelPtr_.reset(new Channel(loop_, sockfd));
    channelPtr_->setWriteCallBack(std::bind(&Connector::handleWrite, shared_from_this()));          // 设置write监听事件，如果一个socket连接状态返回，那么他的write监听会被激活
    channelPtr_->setErrorCallBack(std::bind(&Connector::handleError, shared_from_this()));
    channelPtr_->setCloseCallBack(std::bind(&Connector::handleError, shared_from_this()));
    channelPtr_->enableWriting();
}

int Connector::removeAndResetChannel()
{
    channelPtr_->disableAll();
    channelPtr_->remove();
    int sockfd = channelPtr_->fd();
    // Can't reset channel_ here, because we are inside Channel::handleEvent
    loop_->queueInLoop([this]() { channelPtr_.reset(); });
    return sockfd;
}

void Connector::handleWrite()
{
    if(status_ == Status::Connecting)
    {
        int sockfd = removeAndResetChannel();                               // 移除相应的IOChannel
        int err = Socket::getSocketError(sockfd);                           // 获取socket的错误码
        if(err)
        {
            if(retry_)
            {
                retry(sockfd);                                              // retry尝试
            }
            else
            {
                ::close(sockfd);
            }
            if(errorCallback_)
            {
                errorCallback_();
            }
        }
        else if (Socket::isSelfConnect(sockfd))         
        {
            if (retry_)
            {
                retry(sockfd);
            }
            else
            {
                ::close(sockfd);
            }
            if (errorCallback_)
            {
                errorCallback_();
            }
        }
        else
        {
            status_ = Status::Connected;                                // 连接成功
            if (connect_)
            {
                newConnectionCallback_(sockfd);                         // 调用连接成功回调
            }
            else
            {
                ::close(sockfd);
            }
        }
    }
}

void Connector::handleError()
{
    if (status_ == Status::Connecting)
    {
        status_ = Status::Disconnected;
        int sockfd = removeAndResetChannel();
        int err = Socket::getSocketError(sockfd);
        if (retry_)
        {
            retry(sockfd);
        }
        else
        {
            ::close(sockfd);
        }
        if (errorCallback_)
        {
            errorCallback_();
        }
    }
}

void Connector::retry(int sockfd)
{
    assert(retry_);
    ::close(sockfd);
    status_ = Status::Disconnected;
    if (connect_)
    {
        loop_->runAfter(retryInterval_ / 1000.0,
                        std::bind(&Connector::startInLoop, shared_from_this()));
        retryInterval_ = retryInterval_ * 2;                    // retryInterval会倍增，每次连接超时都会导致下一次的延迟增加
        if (retryInterval_ > maxRetryInterval_)
            retryInterval_ = maxRetryInterval_;
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}

}