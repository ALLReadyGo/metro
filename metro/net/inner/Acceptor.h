#include "metro/net/EventLoop.h"
#include "metro/utils/NonCopyable.h"
#include "metro/net/InetAddress.h"
#include "metro/net/inner/Socket.h"

namespace metro
{
using NewConnectionCallback = std::function<void(int, const InetAddress&)>;
class Acceptor
{
  public:
    Acceptor(EventLoop *loop, 
             const InetAddress &addr,
             bool reUseAddr = true,
             bool reUsePort = true);
    
    ~Acceptor();

    const InetAddress &addr() const
    {
        return addr_;
    }

    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = cb;
    }

    void setNewConnectionCallback(const NewConnectionCallback &&cb)
    {
        newConnectionCallback_ = std::move(cb);
    }

    void listen();
    

  private:
    int idleFd_;                                          // fd占位， 用于file文件描述符耗尽时，关闭acceptor连接时调用
    Socket sock_;                                         
    InetAddress addr_;
    EventLoop *loop_;
    NewConnectionCallback newConnectionCallback_;
    Channel acceptorChannel_;
    void readCallback();
};


}