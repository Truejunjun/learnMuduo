#include "Socket.h"
#include "Logger.h"
#include <unistd.h> // 包含系统调用api

#include <sys/types.h>
#include <sys/socket.h> // bind需要这两个头文件

#include <string.h> // 用到bzero

Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    if ( 0 != ::bind(sockfd_, (sockaddr*)localaddr.getSockAddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd:%d fail \n", sockfd_);
    }
    
}

void Socket::listen()
{
    if ( 0 != ::listen(sockfd_, 1024))
    {
        LOG_FATAL("listen sockfd:%d fail \n", sockfd_);
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    bzero(&addr, sizeof addr);
    int confd = ::accept(sockfd_, (sockaddr*)&addr, &len);
    if (confd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return confd;
}

void Socket::shutdownWrite()
{
    if(::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("");
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SQL_SOCKET, TCP_REUSEADDR, &optval, sizeof optval);
}

void Socket::setReuseport(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SQL_SOCKET, TCP_REUSEPORT, &optval, sizeof optval);
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SQL_SOCKET, TCP_KEEPALIVE, &optval, sizeof optval);
}