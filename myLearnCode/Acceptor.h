#pragma once

#include "noncopyable.h"
#include <functional>
#include "Socket.h"
#include "Channel.h"

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void (int sockfd, const InetAddress&)>;

    Acceptor(EvnetLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        NewConnectionCallback_ = cb;
    }

    bool listenning() const { return listenning_;}
private:
    void handleRead();

    EventLoop *loop_;   // Acceptor使用的就是用户定义的base Loop，也称为main Loop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback NewConnectionCallback_;
    bool listenning_;

};