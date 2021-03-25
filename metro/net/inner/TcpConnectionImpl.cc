#include "metro/net/inner/TcpConnectionImpl.h"
#include <sys/poll.h>
#include <sys/stat.h>
#include <functional>
#include <sys/sendfile.h>

namespace metro
{

TcpConnectionImpl::TcpConnectionImpl(EventLoop *loop,
                                     int socketfd,
                                     const InetAddress &localAddr,
                                     const InetAddress &peerAddr)
  : TcpConnection(),
    loop_(loop),
    ioChannelPtr_(new Channel(loop, socketfd)),
    socketPtr_(new Socket(socketfd)),
    localAddr_(localAddr_),
    peerAddr_(peerAddr)
{
    ioChannelPtr_->setReadCallBack(
        std::bind(&TcpConnectionImpl::readCallback, this));
    ioChannelPtr_->setWriteCallBack(
        std::bind(&TcpConnectionImpl::writeCallback, this));
    ioChannelPtr_->setCloseCallBack(
        std::bind(&TcpConnectionImpl::handleClose, this));
    ioChannelPtr_->setErrorCallBack(
        std::bind(&TcpConnectionImpl::handleError, this));
    socketPtr_->setKeepAlive(true);                                     // 设置socket keep alive
    name_ = localAddr.toIpPort() + "--" + peerAddr.toIpPort();
}

TcpConnectionImpl::~TcpConnectionImpl()
{
}

void TcpConnectionImpl::readCallback()
{
    loop_->assertInLoopThread();
    int ret = 0;
    ssize_t n = readBuffer_.readFd(socketPtr_->fd(), &ret);
    if(n == 0)
    {
        handleClose();
    }
    else if(n < 0)
    {
        if(errno == EPIPE || errno == ECONNRESET)
        {
            LOG_ERROR << "PIPE or CONNRESET" << errno;
        }
        handleClose();
        return;
    }
    if(n > 0)
    {
        extendLife();
        bytesReceived_ += n;
        if(recvMsgCallback_)
        {
            recvMsgCallback_(shared_from_this(), &readBuffer_);
        }
    }
}

void TcpConnectionImpl::writeCallback()
{
    loop_->assertInLoopThread();
    extendLife();
    if(writeBufferList_.empty())
        return;
    assert(ioChannelPtr_->isWriting());
    auto write_buffer = writeBufferList_.front();
    if(write_buffer->sendFd_ < 0)           // fd < 0,表示发送的为buffer
    {
        if(write_buffer->msgBuffer_->readableBytes() <= 0)      // 当前节点并不存在待发送数据
        {
            writeBufferList_.pop_front();
            if(writeBufferList_.empty())
            {
                // ioChannelPtr_->disableWriting(); //HENG
                if(writeCompleteCallback_)
                    writeCompleteCallback_(shared_from_this());
                if(status_ == ConnStatus::Disconnecting)
                {
                    socketPtr_->closeWrite();
                }
            }
            else
            {
                // 按照节点安装规则，不会存在两个连续的buffer节点
                auto fileNode = writeBufferList_.front();
                assert(fileNode->sendFd_ >= 0);
                sendFileInLoop(fileNode);
            }
        }
        else                                                    // 当前节点存在待发送数据
        {
            auto n = writeInLoop(write_buffer->msgBuffer_->peek(),
                                    write_buffer->msgBuffer_->readableBytes());
            if(n >= 0)
            {
                write_buffer->msgBuffer_->retrieve(n);
            }
            else
            {
                if(errno != EWOULDBLOCK)
                {
                    if (errno == EPIPE || errno == ECONNRESET)
                    {
                        LOG_DEBUG << "EPIPE or ECONNRESET, erron="
                                    << errno;
                        return;
                    }
                    LOG_SYSERR << "Unexpected error(" << errno << ")";
                    return;
                }
            }
        }
    }
    else            // file
    {
        if(write_buffer->fileBytesToSend_ <= 0)
        {
            writeBufferList_.pop_front();
            if(writeBufferList_.empty())
            {
                ioChannelPtr_->disableWriting();
                if(writeCompleteCallback_)
                    writeCompleteCallback_(shared_from_this());
                if(status_ == ConnStatus::Disconnecting)
                {
                    socketPtr_->closeWrite();
                }
            }
            else
            {
                if(writeBufferList_.front()->sendFd_ < 0) 
                {

                    // 下一个节点为数据节点
                    // There is data to be sent in the buffer.
                    auto n = writeInLoop(
                        writeBufferList_.front()->msgBuffer_->peek(),
                        writeBufferList_.front()
                            ->msgBuffer_->readableBytes());
                    writeBufferList_.front()->msgBuffer_->retrieve(n);
                    if (n >= 0)
                    {
                        writeBufferList_.front()->msgBuffer_->retrieve(
                            n);
                    }
                    else
                    {
                        if (errno != EWOULDBLOCK)
                        {
                            if (errno == EPIPE || errno == ECONNRESET)
                            {
                                LOG_DEBUG
                                    << "EPIPE or ECONNRESET, erron="
                                    << errno;
                                return;
                            }
                            LOG_SYSERR << "Unexpected error(" << errno
                                        << ")";
                            return;
                        }
                    }
                }
                else
                {
                    sendFileInLoop(writeBufferList_.front());
                }
            }
        }
        else
        {
            sendFileInLoop(write_buffer);
        }
    }
}

void TcpConnectionImpl::sendFileInLoop(const BufferNodePtr &filePtr)
{
    loop_->assertInLoopThread();
    assert(filePtr->sendFd_ >= 0);
    auto bytesSent = sendfile(socketPtr_->fd(),                                   // 零拷贝
                                filePtr->sendFd_,
                                &filePtr->offset_,                                // offset会被内核自动更新
                                filePtr->fileBytesToSend_);
    if (bytesSent < 0)
    {
        if (errno != EAGAIN)
        {
            LOG_SYSERR << "TcpConnectionImpl::sendFileInLoop";
            if (ioChannelPtr_->isWriting())
                ioChannelPtr_->disableWriting();
        }
        return;
    }
    if (bytesSent < filePtr->fileBytesToSend_)
    {
        if (bytesSent == 0)
        {
            LOG_SYSERR << "TcpConnectionImpl::sendFileInLoop";
            return;
        }
    }
    LOG_TRACE << "sendfile() " << bytesSent << " bytes sent";
    filePtr->fileBytesToSend_ -= bytesSent;
    if (!ioChannelPtr_->isWriting())
    {
        ioChannelPtr_->enableWriting();
    }
    return;
}

void TcpConnectionImpl::handleClose()
{
    loop_->assertInLoopThread();
    status_ = ConnStatus::DisConnected;
    ioChannelPtr_->disableAll();
    auto guardThis = shared_from_this();        // timewheel 保护
    if (connectionCallback_)
        connectionCallback_(guardThis);
    if (closeCallback_)
        closeCallback_(guardThis);
}

void TcpConnectionImpl::handleError()
{
    int err = socketPtr_->getSocketError();
    if (err == 0)
        return;
    if (err == EPIPE || err == ECONNRESET || err == 104)
    {
        LOG_DEBUG << err << "\n";
    }
    else
    {
        LOG_ERROR << err << "\n";
    }
}

ssize_t TcpConnectionImpl::writeInLoop(const char *buffer, size_t len)
{
    ssize_t n = ::write(socketPtr_->fd(), buffer, len);
    bytesSend_ += n;
    return n;
}


void TcpConnectionImpl::shutdown()
{
    auto thisPtr = shared_from_this();
    loop_->runInLoop([thisPtr](){
        if(thisPtr->status_ == ConnStatus::Connected)
        {
            thisPtr->status_ = ConnStatus::Disconnecting;
            if(!thisPtr->ioChannelPtr_->isWriting())
            {
                thisPtr->socketPtr_->closeWrite();
            }
        }
    });
}

void TcpConnectionImpl::forceClose()
{
    auto thisPtr = shared_from_this();
    loop_->runInLoop([thisPtr]() {
        if (thisPtr->status_ == ConnStatus::Connected ||
            thisPtr->status_ == ConnStatus::Disconnecting)
        {
            thisPtr->status_ = ConnStatus::Disconnecting;
            thisPtr->handleClose();
        }
    });
}

void TcpConnectionImpl::connectEstablished()
{
    auto thisPtr = shared_from_this();
    loop_->runInLoop([thisPtr]() {
        assert(thisPtr->status_ == ConnStatus::Connecting);
        thisPtr->ioChannelPtr_->tie(thisPtr);
        thisPtr->ioChannelPtr_->enableReading();
        thisPtr->status_ = ConnStatus::Connected;
        if (thisPtr->connectionCallback_)
            thisPtr->connectionCallback_(thisPtr);
    });
}

void TcpConnectionImpl::sendInLoop(const char *buff, size_t length)
{
    loop_->assertInLoopThread();
    if(status_ != ConnStatus::Connected)
        return;
    extendLife();
    size_t remain_len = length;
    ssize_t send_len;
    if(!ioChannelPtr_->isWriting() && writeBufferList_.empty())     
    {
        send_len = writeInLoop(static_cast<const char *>(buff), length);
        if(send_len < 0)
        {
            if (errno != EWOULDBLOCK)
            {
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    LOG_DEBUG << "EPIPE or ECONNRESET, erron=" << errno;
                    return;
                }
                LOG_SYSERR << "Unexpected error(" << errno << ")";
                return;
            }
            send_len = 0;
        }
        remain_len -= send_len;
    }
    if(remain_len > 0)
    {
        if(writeBufferList_.empty() || 
           writeBufferList_.back()->sendFd_ >= 0)               // 需要压入buffer节点
        {
            BufferNodePtr node(new BufferNode);
            node->msgBuffer_ = std::make_shared<MsgBuffer>();
            writeBufferList_.push_back(std::move(node));
        }
        writeBufferList_.back()->msgBuffer_->append(
            buff + send_len, remain_len);
        if(!ioChannelPtr_->isWriting())
            ioChannelPtr_->enableWriting();
        if (highWaterMarkCallback_ &&                           // 发送缓冲区堆积过多
            writeBufferList_.back()->msgBuffer_->readableBytes() >
                highWaterMarkLen_)
        {
            highWaterMarkCallback_(
                shared_from_this(),
                writeBufferList_.back()->msgBuffer_->readableBytes());
        }
    }
    std::cout << "sendInLoop end" << std::string(buff, length) << std::endl;
}

void TcpConnectionImpl::send(const char *msg, size_t len)
{
    if(loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if(sendNum_ == 0)
        {   
            sendInLoop(msg, len);
        }
        else
        {
            ++sendNum_;
            auto buffer = std::make_shared<std::string>(msg, len);
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([buffer, thisPtr](){
                thisPtr->sendInLoop(buffer->data(), buffer->length());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            });
        }
    }
    else
    {
        auto buffer = std::make_shared<std::string>(msg, len);
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([buffer, thisPtr](){
            thisPtr->sendInLoop(buffer->data(), buffer->length());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            --thisPtr->sendNum_;
        });
    }
}


void TcpConnectionImpl::send(const void *msg, size_t len)
{
    if(loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if(sendNum_ == 0)
        {   
            sendInLoop(static_cast<const char*>(msg), len);
        }
        else
        {
            ++sendNum_;
            auto buffer = std::make_shared<std::string>(static_cast<const char *>(msg), len);
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([buffer, thisPtr](){
                thisPtr->sendInLoop(buffer->data(), buffer->length());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            });
        }
    }
    else
    {
        auto buffer = std::make_shared<std::string>(static_cast<const char *>(msg), len);
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([buffer, thisPtr](){
            thisPtr->sendInLoop(buffer->data(), buffer->length());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            --thisPtr->sendNum_;
        });
    }
}

void TcpConnectionImpl::send(const std::string &msg)
{
    if(loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if(sendNum_ == 0)
        {   
            sendInLoop(msg.data(), msg.length());
        }
        else
        {
            ++sendNum_;
            auto buffer = std::make_shared<std::string>(msg);
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([buffer, thisPtr](){
                thisPtr->sendInLoop(buffer->data(), buffer->length());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            });
        }
    }
    else
    {
        auto buffer = std::make_shared<std::string>(msg);
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([buffer, thisPtr](){
            thisPtr->sendInLoop(buffer->data(), buffer->length());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            --thisPtr->sendNum_;
        });
    }
}

void TcpConnectionImpl::send(const std::string &&msg)
{
    if(loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if(sendNum_ == 0)
        {   
            sendInLoop(msg.data(), msg.length());
        }
        else
        {
            ++sendNum_;
            auto buffer = std::make_shared<std::string>(std::move(msg));
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([buffer, thisPtr](){
                thisPtr->sendInLoop(buffer->data(), buffer->length());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            });
        }
    }
    else
    {
        auto buffer = std::make_shared<std::string>(msg);
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([buffer, thisPtr](){
            thisPtr->sendInLoop(buffer->data(), buffer->length());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            --thisPtr->sendNum_;
        });
    }
}

void TcpConnectionImpl::send(const MsgBuffer &buffer)
{
    if(loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if(sendNum_ == 0)
        {   
            sendInLoop(buffer.peek(), buffer.readableBytes());
        }
        else
        {
            ++sendNum_;
            auto buffer_str_ptr = std::make_shared<std::string>(buffer.peek(), buffer.readableBytes());
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([buffer_str_ptr, thisPtr](){
                thisPtr->sendInLoop(buffer_str_ptr->data(), buffer_str_ptr->length());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            });
        }
    }
    else
    {
        auto buffer_str_ptr = std::make_shared<std::string>(buffer.peek(), buffer.readableBytes());
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([buffer_str_ptr, thisPtr](){
            thisPtr->sendInLoop(buffer_str_ptr->data(), buffer_str_ptr->length());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            --thisPtr->sendNum_;
        });
    }
}

void TcpConnectionImpl::send(MsgBuffer &&buffer)
{
    if(loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if(sendNum_ == 0)
        {   
            sendInLoop(buffer.peek(), buffer.readableBytes());
        }
        else
        {
            ++sendNum_;
            auto buffer_str_ptr = std::make_shared<std::string>(buffer.peek(), buffer.readableBytes());
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([buffer_str_ptr, thisPtr](){
                thisPtr->sendInLoop(buffer_str_ptr->data(), buffer_str_ptr->length());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([buffer = std::move(buffer), thisPtr](){
            thisPtr->sendInLoop(buffer.peek(), buffer.readableBytes());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            --thisPtr->sendNum_;
        });
    }
}

void TcpConnectionImpl::send(const std::shared_ptr<std::string> &msgPtr)
{
    send(*msgPtr);
}

void TcpConnectionImpl::send(const std::shared_ptr<MsgBuffer> &msgPtr)
{
    send(*msgPtr);
}

void TcpConnectionImpl::sendFile(const char *filename,
                                 size_t offset, 
                                 size_t length)
{
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        LOG_SYSERR << filename << " open error";
        return;
    }

    if (length == 0)
    {
        struct stat filestat;
        if (stat(filename, &filestat) < 0)
        {
            LOG_SYSERR << filename << " stat error";
            close(fd);
            return;
        }
        length = filestat.st_size;
    }
    sendFile(fd, offset, length);
}

void TcpConnectionImpl::sendFile(int fd,
                                 size_t offset, 
                                 size_t length)
{
    assert(length > 0);
    assert(fd > 0);
    BufferNodePtr node_ptr = std::make_shared<BufferNode>();
    node_ptr->sendFd_ = fd;
    node_ptr->offset_ = offset;
    node_ptr->fileBytesToSend_ = length;
    if(loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if(sendNum_ == 0)
        {
            writeBufferList_.push_back(node_ptr);
            if (writeBufferList_.size() == 1)
            {
                sendFileInLoop(writeBufferList_.front());
                return;
            }
        }
        else
        {
            ++sendNum_;
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, node_ptr]() {
                thisPtr->writeBufferList_.push_back(node_ptr);
                {
                    std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                    --thisPtr->sendNum_;
                }

                if (thisPtr->writeBufferList_.size() == 1)
                {
                    thisPtr->sendFileInLoop(thisPtr->writeBufferList_.front());
                }
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([thisPtr, node_ptr]() {
            thisPtr->writeBufferList_.push_back(node_ptr);
            {
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            }

            if (thisPtr->writeBufferList_.size() == 1)
            {
                thisPtr->sendFileInLoop(thisPtr->writeBufferList_.front());
            }
        });
    }
}


}