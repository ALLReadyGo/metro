#include "metro/net/InetAddress.h"
#include <cstring>
#include <iostream>

static const in_addr_t kInAddrAny = INADDR_ANY;
static const in_addr_t kINAddrLoopbcak = INADDR_LOOPBACK;

namespace metro
{

InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6)
  : isIpv6_(ipv6)
{
    if(ipv6)
    {
        memset(&addr6_, 0, sizeof(addr6_));
        addr6_.sin6_family = AF_INET6;
        addr6_.sin6_port = htons(port);
        addr6_.sin6_addr = loopbackOnly ? in6addr_loopback : in6addr_any;
    }
    else
    {
        memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        in_addr_t ip = loopbackOnly ? kINAddrLoopbcak : kInAddrAny;
        addr_.sin_addr.s_addr = htonl(ip);
    }
}

InetAddress::InetAddress(const std::string &ip, uint16_t port, bool ipv6) 
  : isIpv6_(ipv6)
{
    if(ipv6)
    {
        memset(&addr6_, 0, sizeof(addr6_));
        addr6_.sin6_family = AF_INET6;
        addr6_.sin6_port = htons(port);
        if(inet_pton(AF_INET6, ip.c_str(), &addr6_.sin6_addr) < 0) 
        {
            std::cout << "ip address error";
            abort();
        }
    }
    else
    {
        memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        if(inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) < 0)
        {
            std::cout << "ip address error";
            abort();
        }
    }
}

std::string InetAddress::toIp() const 
{
    char buff[64] = "";
    if(addr_.sin_family == AF_INET) 
    {
        inet_ntop(AF_INET, &addr_.sin_addr, buff, sizeof(buff));
    }
    else if(addr_.sin_family == AF_INET6)
    {
        inet_ntop(AF_INET6, &addr6_.sin6_addr, buff, sizeof(buff));
    }
    return std::string(buff);
}

std::string InetAddress::toPort() const 
{
    uint16_t port = ntohs(addr_.sin_port);
    return std::to_string(port);
}

std::string InetAddress::toIpPort() const
{
    return toIp() + " : " + toPort();
}

bool InetAddress::isIntranetIp() const
{
    if(addr_.sin_family == AF_INET)
    {
        uint32_t ip_addr = ntohl(addr_.sin_addr.s_addr);
        if ((ip_addr >= 0x0A000000 && ip_addr <= 0x0AFFFFFF) ||
            (ip_addr >= 0xAC100000 && ip_addr <= 0xAC1FFFFF) ||
            (ip_addr >= 0xC0A80000 && ip_addr <= 0xC0A8FFFF) ||
            ip_addr == 0x7f000001)
        {
            return true;
        }
    }
    else
    {
        auto addrP = ip6NetEndian();
        if (*addrP == 0 && *(addrP + 1) == 0 && *(addrP + 2) == 0 &&
            ntohl(*(addrP + 3)) == 1)
            return true;
        auto i32 = (ntohl(*addrP) & 0xffc00000);
        if(i32 == 0xfec00000 || i32 == 0xfe800000)
            return true;
        if(*addrP == 0 && *(addrP + 1) == 0 && ntohl(*(addrP + 2)) == 0xffff)
        {
            uint32_t ip_addr = ntohl(*(addrP + 3));
            if ((ip_addr >= 0x0A000000 && ip_addr <= 0x0AFFFFFF) ||
                (ip_addr >= 0xAC100000 && ip_addr <= 0xAC1FFFFF) ||
                (ip_addr >= 0xC0A80000 && ip_addr <= 0xC0A8FFFF) ||
                ip_addr == 0x7f000001)
            {
                return true;
            }
        }
    }
    return false;
}

bool InetAddress::isLoopbackIp() const
{
    if(!isIpv6())
    {
        uint32_t ip_addr = ntohl(addr_.sin_addr.s_addr);
        if (ip_addr == 0x7f000001)
        {
            return true;
        }
    }
    else
    {
        auto addrP = ip6NetEndian();
        if(*addrP == 0 && *(addrP + 1) == 0 && *(addrP + 2) == 0 &&
            ntohl(*(addrP + 3)) == 1)
            return true;
        // the IPv6 version of an IPv4 loopback address
        if(*addrP == 0 && *(addrP + 1) == 0 && ntohl(*(addrP + 2)) == 0xffff &&
            ntohl(*(addrP + 3)) == 0x7f000001)
            return true;
    }
    return false;
}

uint32_t InetAddress::ipNetEndian() const
{
    return addr_.sin_addr.s_addr;
}

const uint32_t *InetAddress::ip6NetEndian() const
{
    return addr6_.sin6_addr.s6_addr32;
}

}