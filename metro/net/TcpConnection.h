/**
 * socket建立连接完成之后的辅助类，这是个基类，具体实现为TcpConnectionImpl
 * 这个类中封装了对连接socket的所有操作
 * 关键接口是read和write
 */
#pragma once
#include <iostream>
#include <memory>
#include "metro/net/InetAddress.h"
#include "metro/net/EventLoop.h"
#include "metro/utils/MsgBuffer.h"
#include "metro/net/callbacks.h"

namespace metro
{

class TcpConnection
{
  public:
    TcpConnection();

    virtual ~TcpConnection();

    // 数据发送接口
    virtual void send(const char *msg, size_t len) = 0;     
    virtual void send(const void *msg, size_t len) = 0;
    virtual void send(const std::string &msg) = 0;
    virtual void send(const std::string &&msg) = 0;
    virtual void send(const MsgBuffer &buffer) = 0;
    virtual void send(MsgBuffer &&buffer) = 0;
    virtual void send(const std::shared_ptr<std::string> &msgPtr) = 0;
    virtual void send(const std::shared_ptr<MsgBuffer> &msgPtr) = 0;
    // sendfile接口，零拷贝数据发送
    virtual void sendFile(const char *filename,
                          size_t offset = 0, 
                          size_t length = 0) = 0;
    // 获取对端和自己的socket地址
    virtual const InetAddress &localAddr() const = 0;
    virtual const InetAddress &peerAddr() const = 0;

    virtual bool connected() = 0;

    virtual bool disConnected() = 0;
    // 获取接收数据的缓冲区
    virtual MsgBuffer *getRecvBuffer() = 0;

    virtual void setHighWaterMarkCallBack(const HighWaterMarkCallback &cb, 
                                          size_t markLen = 0) = 0;
    
    virtual void setTcpNoDelay(bool on) = 0;

    virtual void shutdown() = 0;

    virtual void forceClose() = 0;

    virtual EventLoop* getLoop() = 0;

    void setContext(const std::shared_ptr<void> &context)
    {
        contextPtr_ = context;
    }

    void setContext(std::shared_ptr<void> &&context)
    {
        contextPtr_ = std::move(context);
    }

    template<typename T>
    std::shared_ptr<T> getContext() const
    {
        return std::static_pointer_cast<T>();
    }
    
    bool hasContext()
    {
        return (bool)contextPtr_;
    }

    void clearContext()
    {
        contextPtr_.reset();
    }

    virtual void keepAlive() = 0;

    virtual bool isKeepAlive() = 0;
    
    virtual size_t bytesSend() const = 0;

    virtual size_t bytesRecved() const = 0;
    
  private:
    std::shared_ptr<void> contextPtr_;              
};

}