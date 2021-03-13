#include "metro/net/InetAddress.h"
#include <iostream>

/*
 * g++ metro/net/InetAddress.cc metro/tests/InetAddress_test.cc -I./
*/

int main() 
{
    metro::InetAddress addr4("192.168.0.1", 80);
    std::cout << addr4.toIpPort() << std::endl;

    metro::InetAddress addr4_any(80);
    std::cout << addr4_any.toIpPort() << std::endl;

    metro::InetAddress addr4_loopback(80, true);
    std::cout << addr4_loopback.toIpPort() << std::endl;

    metro::InetAddress addr6("2001:dd8:216:2104:f147:8b63:b74d:bc81", 80, true);
    std::cout << addr6.toIpPort() << std::endl;

    metro::InetAddress addr6_any(80, false, true);
    std::cout << addr6_any.toIpPort() << std::endl;

    metro::InetAddress addr6_loopback(80, true, true);
    std::cout << addr6_loopback.toIpPort() << std::endl;
}