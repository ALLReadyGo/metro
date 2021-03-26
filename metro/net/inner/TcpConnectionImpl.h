#pragma once

#include <memory>
#include "metro/utils/NonCopyable.h"
#include "metro/net/inner/Socket.h"
#include "metro/utils/TimingWheel.h"
#include "metro/net/TcpConnection.h"

#include <sys/unistd.h>

namespace metro
{

class TcpConnectionImpl : public TcpConnection,
                          public NonCopyable,
                          public std::enable_shared_from_this<TcpConnectionImpl>
{
    friend class TcpServer;
  public:
    TcpConnectionImpl(EventLoop *loop,
                      int sockfd,
                      const InetAddress &localAddr,
                      const InetAddress &peerAddr);

    virtual ~TcpConnectionImpl() override;

    /* 所有send操作支持异步写入 */
    virtual void send(const char *msg, size_t len) override;
    virtual void send(const void *msg, size_t len) override;
    virtual void send(const std::string &msg) override;
    virtual void send(const std::string &&msg) override;
    virtual void send(const MsgBuffer &buffer) override;
    virtual void send(MsgBuffer &&buffer) override;
    virtual void send(const std::shared_ptr<std::string> &msgPtr) override;
    virtual void send(const std::shared_ptr<MsgBuffer> &msgPtr) override;

    virtual void sendFile(const char *filename,
                          size_t offset = 0, 
                          size_t length = 0) override;

    void sendFile(int fd,
                  size_t offset = 0, 
                  size_t length = 0);

    virtual const InetAddress &localAddr() const override
    {
        return localAddr_;
    }

    virtual const InetAddress &peerAddr() const override
    {
        return peerAddr_;
    }

    virtual bool connected() override
    {
        return status_ == ConnStatus::Connected;
    }

    virtual bool disConnected() override
    {
        return status_ == ConnStatus::DisConnected;
    }

    virtual void setHighWaterMarkCallBack(const HighWaterMarkCallback &cb, 
                                        size_t markLen = 0) override
    {
        highWaterMarkCallback_ = cb;
        highWaterMarkLen_ = markLen;
    }

    virtual MsgBuffer *getRecvBuffer() override
    {
        return &readBuffer_;
    }

    virtual void setTcpNoDelay(bool on) override
    {
        socketPtr_->setTcpNoDelay(on);
    }

    void connectDestroyed();

    virtual void shutdown() override;

    virtual void forceClose() override;

    virtual EventLoop* getLoop() override
    {
        return loop_;
    }

    virtual void keepAlive() override
    {
        idleTimeout_ = 0;
        auto entry = kickoffEntry_.lock();
        if(entry)
        {
            entry->reset();
        }
    }

    virtual bool isKeepAlive() override
    {
        return idleTimeout_ == 0;
    }
    
    virtual size_t bytesSend() const override
    {
        return bytesSend_;
    }

    virtual size_t bytesRecved() const override
    {
        return bytesReceived_;
    }

    void setRecvMsgCallback(const RecvMessageCallback &cb)
    {
        recvMsgCallback_ = cb;
    }

    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connectionCallback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }

    void setCloseCallback(const CloseCallback &cb)
    {
        closeCallback_ = cb;
    }
    
    void connectEstablished();

  protected:
    
    enum class ConnStatus
    {
        DisConnected = 0,
        Connecting,
        Connected,
        Disconnecting
    };

    struct BufferNode
    {
        int sendFd_{-1};
        off_t offset_;
        ssize_t fileBytesToSend_;
        std::shared_ptr<MsgBuffer> msgBuffer_;
        ~BufferNode()
        {
            if(sendFd_ >= 0)
            {
                close(sendFd_);
            }
        }
    };

    class KickoffEntry
    {
      public:
        explicit KickoffEntry(const std::weak_ptr<TcpConnection> &conn)
          : conn_(conn)
        {
        }

        void reset()
        {
            conn_.reset();
        }

        ~KickoffEntry()
        {
            auto conn = conn_.lock();
            if(conn)
            {
                conn->forceClose();
            }
        }
      private:
        std::weak_ptr<TcpConnection> conn_;
    };

    /* kickoff */
    std::weak_ptr<KickoffEntry> kickoffEntry_;
    std::weak_ptr<TimingWheel> timingWheelWeakPtr_;
    size_t idleTimeout_;
    Date lastTimingWheelUpdateTime_;
    
    void enableKickoffEntry(size_t timeout,
                            const std::shared_ptr<TimingWheel> &timingWheel)
    {
        assert(timingWheel);
        assert(timingWheel->getLoop() == loop_);
        assert(timeout > 0);
        auto entry = std::make_shared<KickoffEntry>(shared_from_this());
        kickoffEntry_ = entry;
        idleTimeout_ = timeout;
        timingWheel->insertEntry(timeout, entry);
    }

    void extendLife()
    {
        if(idleTimeout_ <= 0)
            return;
        auto now = Date::now();
        if(lastTimingWheelUpdateTime_.after(1) > now)       // 避免插入过多
            return;
        lastTimingWheelUpdateTime_ = now;
        auto entry = kickoffEntry_.lock();
        if(entry)
        {
            auto timingWheelPtr = timingWheelWeakPtr_.lock();
            if(timingWheelPtr)
            {
                timingWheelPtr->insertEntry(idleTimeout_, timingWheelPtr);
            }
        }
    }
    
    
    using BufferNodePtr = std::shared_ptr<BufferNode>;

    /* send */
    void sendInLoop(const char *buffer, size_t length);

    EventLoop *loop_;
    std::unique_ptr<Channel> ioChannelPtr_;
    std::unique_ptr<Socket> socketPtr_;
    MsgBuffer readBuffer_;
    std::list<BufferNodePtr> writeBufferList_;
    // ioChannel Callback
    void readCallback();
    void writeCallback();
    InetAddress localAddr_, peerAddr_;
    ConnStatus status_{ConnStatus::Connecting};

    // establish
    

    // callback
    RecvMessageCallback recvMsgCallback_;
    ConnectionCallback connectionCallback_;
    CloseCallback closeCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    // high water
    size_t highWaterMarkLen_;
    HighWaterMarkCallback highWaterMarkCallback_;

    void handleClose();
    void handleError();

    // writeInLoop
    ssize_t writeInLoop(const char *buffer, size_t len);
    void sendFileInLoop(const BufferNodePtr &filePtr);

    std::mutex sendNumMutex_;
    size_t sendNum_{0};             // 记录当前有多少没有进入WriteBufferList的写请求
    
    size_t bytesSend_{0};
    size_t bytesReceived_{0};

    std::string name_;
};

}