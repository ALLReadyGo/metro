#include "metro/net/inner/Socket.h"
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>

namespace metro
{

bool Socket::isSelfConnect(int sockfd)
{
    struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
    struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
    if (localaddr.sin6_family == AF_INET)
    {
        const struct sockaddr_in *laddr4 =
            reinterpret_cast<struct sockaddr_in *>(&localaddr);
        const struct sockaddr_in *raddr4 =
            reinterpret_cast<struct sockaddr_in *>(&peeraddr);
        return laddr4->sin_port == raddr4->sin_port &&
               laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
    }
    else if (localaddr.sin6_family == AF_INET6)
    {
        return localaddr.sin6_port == peeraddr.sin6_port &&
               memcmp(&localaddr.sin6_addr,
                      &peeraddr.sin6_addr,
                      sizeof localaddr.sin6_addr) == 0;
    }
    else
    {
        return false;
    }
}

struct sockaddr_in6 Socket::getLocalAddr(int sockfd)
{
    struct sockaddr_in6 localaddr;
    memset(&localaddr, 0, sizeof(localaddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if (::getsockname(sockfd,
                      static_cast<struct sockaddr *>((void *)(&localaddr)),
                      &addrlen) < 0)
    {
        LOG_SYSERR << "sockets::getLocalAddr";
    }
    return localaddr;
}

struct sockaddr_in6 Socket::getPeerAddr(int sockfd)
{
    struct sockaddr_in6 peeraddr;
    memset(&peeraddr, 0, sizeof(peeraddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    if (::getpeername(sockfd,
                      static_cast<struct sockaddr *>((void *)(&peeraddr)),
                      &addrlen) < 0)
    {
        LOG_SYSERR << "sockets::getPeerAddr";
    }
    return peeraddr;
}


void Socket::bindAddress(const InetAddress &localAddr)
{
    assert(sockFd_ > 0);
    int ret;
    if (localAddr.isIpv6())
        ret = ::bind(sockFd_, localAddr.getSockAddr(), sizeof(sockaddr_in6));
    else
        ret = ::bind(sockFd_, localAddr.getSockAddr(), sizeof(sockaddr_in));

    if (ret == 0)
        return;
    else
    {
        LOG_SYSERR << ", Bind address failed at " << localAddr.toIpPort();
        exit(1);
    }
}

void Socket::listen()
{
    assert(sockFd_ > 0);
    int ret = ::listen(sockFd_, SOMAXCONN);
    if (ret < 0)
    {
        LOG_SYSERR << "listen failed";
        exit(1);
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    struct sockaddr_in6 addr6;
    memset(&addr6, 0, sizeof(addr6));
    socklen_t size = sizeof(addr6);
    int connfd = ::accept4(sockFd_,
                           (struct sockaddr *)&addr6,
                           &size,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0)
    {
        peeraddr->setSockAddr6(addr6);
    }
    return connfd;
}

void Socket::closeWrite()
{
    if(::shutdown(sockFd_, SHUT_WR) < 0)
    {
        LOG_ERROR << "close error";
    }
}

int Socket::read(char *buffer, uint64_t len)
{
    return ::read(sockFd_, buffer, len);
}   
    

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockFd_,
                 IPPROTO_TCP,
                 TCP_NODELAY,
                 &optval,
                 static_cast<socklen_t>(sizeof optval));
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockFd_,
                 SOL_SOCKET,
                 SO_REUSEADDR,
                 &optval,
                 static_cast<socklen_t>(sizeof optval));
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;

    int ret = ::setsockopt(sockFd_,
                           SOL_SOCKET,
                           SO_REUSEPORT,
                           &optval,
                           static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
        LOG_SYSERR << "SO_REUSEPORT failed.";
    }
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockFd_,
                 SOL_SOCKET,
                 SO_KEEPALIVE,
                 &optval,
                 static_cast<socklen_t>(sizeof optval));
}

int Socket::getSocketError()
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);
    if (::getsockopt(sockFd_, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else
    {
        return optval;
    }
}

}