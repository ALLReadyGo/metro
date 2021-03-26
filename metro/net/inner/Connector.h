#pragma once

#include "metro/net/Channel.h"
#include "metro/net/EventLoop.h"
#include "metro/utils/NonCopyable.h"
#include "metro/net/inner/Socket.h"
#include <memory>

namespace metro
{

class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;

class Connector : public NonCopyable,
                  public std::enable_shared_from_this<Connector>
{
  public:
    using NewConnetionCallback = std::function<void(int sockfd)>;
    using ConnetionErrorCallback = std::function<void()>;

    Connector(EventLoop *loop, const InetAddress &addr, bool retry = true);
    Connector(EventLoop *loop, InetAddress &&addr, bool retry = true);

    void start();
    void restart();
    void stop();

    void setNewConnectionCallback(const NewConnetionCallback &callback)
    {
        newConnectionCallback_ = callback;
    }

    void setNewConnectionCallback(NewConnetionCallback &&callback)
    {
        newConnectionCallback_ = std::move(callback);
    }

    void setConnetionErrorCallback(const ConnetionErrorCallback &callback)
    {
        errorCallback_ = callback;
    }

    void setConnetionErrorCallback(ConnetionErrorCallback &&callback)
    {
        errorCallback_ = std::move(callback);
    }

    const InetAddress &serverAddress() const
    {
        return serverAddr_;
    }

  private:
    NewConnetionCallback newConnectionCallback_;
    ConnetionErrorCallback errorCallback_;
    enum class Status
    {
        Disconnected = 0,
        Connecting,
        Connected
    };
    static constexpr int kMaxRetryDelayMs = 30 * 1000;
    static constexpr int kInitRetryDelayMs = 500;
    std::shared_ptr<Channel> channelPtr_;
    EventLoop *loop_;
    InetAddress serverAddr_;

    std::atomic_bool connect_{false};
    std::atomic<Status> status_{Status::Disconnected};

    int retryInterval_{kInitRetryDelayMs};
    int maxRetryInterval_{kMaxRetryDelayMs};

    bool retry_;
    void startInLoop();
    void connect();
    void connecting(int sockfd);
    int removeAndResetChannel();
    void handleWrite();
    void handleError();
    void retry(int sockfd);
};

}
