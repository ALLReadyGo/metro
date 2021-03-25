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
    loop_->runInLoop([this]() { startInLoop(); });
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

void Connector::connect()
{
    int sockfd = Socket::createNonblockingSocketOrDie(serverAddr_.family());
    errno = 0;
    int ret = Socket::connect(sockfd, serverAddr_);
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            LOG_TRACE << "connecting";
            connecting(sockfd);
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
    channelPtr_->setWriteCallBack(std::bind(&Connector::handleWrite, shared_from_this()));
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
        int sockfd = removeAndResetChannel();
        int err = Socket::getSocketError(sockfd);
        if(err)
        {
            if(retry_)
            {
                retry(sockfd);
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
        else if (Socket::isSelfConnect(sockfd))         // TODO:这个是什么现象
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
            status_ = Status::Connected;
            if (connect_)
            {
                newConnectionCallback_(sockfd);
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
        retryInterval_ = retryInterval_ * 2;
        if (retryInterval_ > maxRetryInterval_)
            retryInterval_ = maxRetryInterval_;
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}

}