#pragma once
#include <functional>
#include <memory>

namespace metro
{

class MsgBuffer;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using Func = std::function<void()>;
using TimerCallback = std::function<void()>;
using RecvMessageCallback =
    std::function<void(const TcpConnectionPtr &, MsgBuffer *)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using HighWaterMarkCallback = 
    std::function<void(const TcpConnectionPtr &, size_t)>;


}