#pragma once
#include <set>
#include <map>
#include "metro/utils/TimingWheel.h"
#include "metro/utils/NonCopyable.h"
#include "metro/net/EventLoop.h"
#include "metro/net/InetAddress.h"
#include "metro/net/EventLoopThreadPool.h"
#include "metro/net/inner/Acceptor.h"
#include <assert.h>

namespace metro
{

class TcpServer : NonCopyable
{
  public:
    TcpServer(EventLoop *loop,
              const InetAddress &address,
              const std::string &name,
              bool reUseAddr = true,
              bool reUsePort = true);
    ~TcpServer();

    void start();

    void stop();

    void setIoLoopNum(size_t num)
    {
        assert(!started_);
        loopPoolPtr_ = std::make_shared<EventLoopThreadPool>(num);
        loopPoolPtr_->start();
    }

    void setIoLoopThreadPool(const std::shared_ptr<EventLoopThreadPool> &pool)
    {
        assert(pool->size() > 0);
        assert(!started_);
        loopPoolPtr_ = pool;
        loopPoolPtr_->start();
    }

    void setRecvMessageCallback(const RecvMessageCallback &cb)
    {
        recvMessageCallback_ = cb;
    }
    void setRecvMessageCallback(const RecvMessageCallback &&cb)
    {
        recvMessageCallback_ = std::move(cb);
    }
    void setConnetionCallback(const ConnectionCallback &cb)
    {
        connectionCallback_ = cb;
    }
    void setConnetionCallback(const ConnectionCallback &&cb)
    {
        connectionCallback_ = std::move(cb);
    }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }
    void setWriteCompleteCallback(WriteCompleteCallback &&cb)
    {
        writeCompleteCallback_ = std::move(cb);
    }
    const std::string &name() const
    {
        return serverName_;
    }
    const std::string ipPort() const;

    const metro::InetAddress &address() const;

    EventLoop *getLoop() const
    {
        return loop_;
    }

    std::vector<EventLoop *> getIoLoops() const
    {
        return loopPoolPtr_->getLoops();
    }

    void kickoffIdleConnections(size_t timeout)
    {
        loop_->runInLoop([this, timeout]() {
            assert(!started_);
            idleTimeOut_ = timeout;
        });
    }
    
  private:
    EventLoop *loop_;
    bool started_{false};
    std::unique_ptr<Acceptor> acceptorPtr_;
    void newConnection(int fd, const InetAddress &peer);
    std::string serverName_;
    std::set<TcpConnectionPtr> connSet_;

    RecvMessageCallback recvMessageCallback_;
    ConnectionCallback connectionCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    size_t idleTimeOut_{0};
    std::map<EventLoop *, std::shared_ptr<TimingWheel>> timingWheelMap_;
    void connectionClosed(const TcpConnectionPtr &connPtr);
    std::shared_ptr<EventLoopThreadPool> loopPoolPtr_;


};


}