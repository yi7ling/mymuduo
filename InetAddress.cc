#include "InetAddress.h"
#include "Logger.h"

#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    memset(&addr_, 0, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
    addr_.sin_port = htons(port); // 将主机字节序 转为 网络字节序 Host To Network Short
}

std::string InetAddress::toIp() const
{
    char buf[64] = "";

    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}

std::string InetAddress::toIpPort() const
{
    // ip:port
    char buf[64] = "";

    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port); // network to host
    sprintf(buf+end, ":%u", port);
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}

/*
#include <iostream>

int main()
{
    InetAddress inetadd(8080, "192.168.101.1");

    // LOG_INFO("ipdao:%s", "12312312");
    LOG_INFO("ip-port:%s, ip:%s, port:%u \n",
        inetadd.toIpPort().c_str(), inetadd.toIp().c_str(), inetadd.toPort());
}
*/