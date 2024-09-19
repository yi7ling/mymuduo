#pragma once

#include <netinet/in.h>
#include <string>

/**
 * 主要是对socket编程中sockaddr_in进行封装
 * 使其变为更友好的简单接口
 */
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");

    explicit InetAddress(const struct sockaddr_in addr)
        : addr_(addr)
    {}

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    // 将 sockaddr_in 转成 sockaddr
    const struct sockaddr* getSockAddr() const {return static_cast<const struct sockaddr*>((const void*)(&addr_));}

    void setSockAddr(const sockaddr_in addr) { addr_ = addr; };
private:
    struct sockaddr_in addr_;// ipv4
};