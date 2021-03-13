
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <cstdint>

namespace metro
{

class InetAddress
{
  public:
    InetAddress(uint16_t port,
                bool loopbackOnly = false, 
                bool ipv6 = false);

    InetAddress(const std::string &ip, 
                u_int16_t port, 
                bool ipv6 = false);

    explicit InetAddress(const sockaddr_in &addr)
      : addr_(addr) 
    {
    }

    explicit InetAddress(const sockaddr_in6 &addr6)
      : addr6_(addr6), isIpv6_(true) 
    {
    }
    
    sa_family_t family() const
    {
        return addr_.sin_family;   
    }

    std::string toIp() const;

    std::string toPort() const;
    
    std::string toIpPort() const;

    bool isIpv6() const
    {
        return isIpv6_;
    }

    bool isIntranetIp() const;

    bool isLoopbackIp() const;

    const struct sockaddr *getSockAddr() const
    {
        return reinterpret_cast<const struct sockaddr *>(&addr_);
    }

    void setSockAddr6(const struct sockaddr_in6* addr6)
    {
        addr6_ = *addr6;
        isIpv6_ = addr6->sin6_family == AF_INET6;
    }

    void setSockAddr(const struct sockaddr_in *addr)
    {
        addr_ = *addr;
        isIpv6_ = false;
    }

    uint32_t ipNetEndian() const;

    const uint32_t *ip6NetEndian() const;

    u_int16_t portNetEndian() const
    {
        return addr_.sin_port;
    }

    void setPortNetEndian(u_int16_t port)
    {
        addr_.sin_port = port;
    }

  private:
    union 
    {
        struct sockaddr_in addr_;
        struct sockaddr_in6 addr6_;
    };
    bool isIpv6_ = false;
};


}