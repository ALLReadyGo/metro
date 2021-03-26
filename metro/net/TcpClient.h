#pragma once
#include "metro/net/EventLoop.h"
#include "metro/utils/NonCopyable.h"
#include "metro/net/TcpConnection.h"
#include "metro/net/inner/Connector.h"
#include "metro/net/callbacks.h"

namespace metro
{

class TcpClient : public NonCopyable
{
  public:
    TcpClient(EventLoop *loop,
              const InetAddress &service,
              const std::string &name);
    
    ~TcpClient();

    void connect();

    void disconnect();

    void stop();

    TcpConnectionPtr connecion() const;

    EventLoop *getLoop() const
    {
        return loop_;
    }

    bool retry()
    {
        return retry_;
    }

    void enableRetry()
    {
        retry_ = true;
    }

    const std::string &name()
    {
        return name_;
    }

    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connectionCallback_ = cb;
    }

    void setConnectionCallback(ConnectionCallback &&cb)
    {
        connectionCallback_ = std::move(cb);
    }

    void setConnectionErrorCallback(const ConnectionErrorCallback &cb)
    {
        connectionErrorCallback_ = cb;
    }

    void setRecvMessageCallback(const RecvMessageCallback &cb)
    {
        recvMessageCallback_ = cb;
    }
    void setRecvMessageCallback(RecvMessageCallback &&cb)
    {
        recvMessageCallback_ = std::move(cb);
    }

    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }

    void setWriteCompleteCallback(WriteCompleteCallback &&cb)
    {
        writeCompleteCallback_ = std::move(cb);
    }

  private:
    void newConnection(int sockfd);
    void removeConnection(const TcpConnectionPtr &connPtr);

    EventLoop *loop_;
    ConnectorPtr connector_;
    const std::string name_;
    ConnectionCallback connectionCallback_;
    ConnectionErrorCallback connectionErrorCallback_;
    RecvMessageCallback recvMessageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    std::atomic_bool retry_;
    std::atomic_bool connect_;

    mutable std::mutex mutex_;
    TcpConnectionPtr connection_;
};


}