/**
 * socket相关接口封装
 */ 
#pragma once
#include <fcntl.h>
#include "metro/net/InetAddress.h"
#include "metro/utils/NonCopyable.h"
#include "metro/utils/Logger.h"
namespace metro
{

class Socket : public NonCopyable
{
  public:
    static int createNonblockingSocketOrDie(int family)
    {
        int sockfd = socket(family,
                          SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                          IPPROTO_TCP);
        if(sockfd < 0)
        {
            LOG_FATAL << "socket creat error";
            exit(1);
        }
        return sockfd;
    }

    static int getSocketError(int sockfd)
    {
        int optval;
        socklen_t optlen = static_cast<socklen_t>(sizeof optval);
        if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
        {
            return errno;
        }
        else
        {
            return optval;
        }
    }

    static int connect(int sockfd, const InetAddress &addr)
    {
        if(addr.isIpv6())
        {
            return ::connect(sockfd, 
                             addr.getSockAddr(),
                             static_cast<socklen_t>(sizeof(sockaddr_in6)));
        }
        else
        {
            return ::connect(sockfd, 
                             addr.getSockAddr(),
                             static_cast<socklen_t>(sizeof(sockaddr_in)));
        }
    }

    static void setNonBlockAndCloseOnExec(int sockfd)
    {
        int flags = ::fcntl(sockfd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        int ret = ::fcntl(sockfd, F_SETFL, flags);

        flags = ::fcntl(sockfd, F_GETFD, 0);
        flags |= FD_CLOEXEC;
        ret = ::fcntl(sockfd, F_SETFD, flags);
    }

    static bool isSelfConnect(int sockfd);

    static struct sockaddr_in6 getLocalAddr(int sockfd);
    static struct sockaddr_in6 getPeerAddr(int sockfd);



    explicit Socket(int sockfd)
      : sockFd_(sockfd)
    {
    }

    ~Socket()
    {
    }

    void bindAddress(const InetAddress &localAddr);
    void listen();
    int accept(InetAddress *peeraddr);
    void closeWrite();
    int read(char *buffer, uint64_t len);
    int fd()
    {
        return sockFd_;
    }

    void setTcpNoDelay(bool on);

    void setReuseAddr(bool on);

    void setReusePort(bool on);

    void setKeepAlive(bool on);

    int getSocketError();

  private:
    int sockFd_;
    
};

}