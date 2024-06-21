#pragma once
#include "noncopyable.h"

class InetAddress;

class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {
        
    }

    ~Socket();

    int fd() const { return sockfd_; }  // 只读方法，加上const关键字
    void bindAddress(const InetAddress &localaddr);
    void listen();

    int accept(InetAddress *peeraddr);
    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReuseport(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};