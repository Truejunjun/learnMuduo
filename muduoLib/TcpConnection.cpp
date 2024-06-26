#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <funcitonal>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h> // bind需要这两个头文件
#include <string.h> // 用到bzero
#include <netinet/tcp.h> // IPPROTO_TCP宏定义
#include <string>

static EventLoop* checkLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("TcpConnection is null. \n");
    }
    return loop;
}


TcpConnection::TcpConnection(EventLoop *loop, const std::string &name, int sockfd,
                    const InetAddress& localAddr_, const InetAddress& peerAddr_)
        : loop_(checkLoopNotNull(loop)), name_(nameArg), state_(kConnecting),
        reading_(true), socket_(new Socket(sockfd)), channel_(new Channel(loop, sockfd)),
        localAddr_(localAddr), peerAddr_(peerAddr), highWaterMark_(64*1024*1024)   // 64M
{
    // 给channel设置回调函数，poller给channel通知感兴趣的事件发生了，channel会进行相应的回调操作
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d \n", name.c_str(), sockfd);
    socket_->setKeepAlive(true);    // 启动了保活机制
}

// 因为使用的成员socket和channel都是智能指针，所以无需手动操作
TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", name_.c_str(), channel_->fd(), (int)state_);
}


void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t  n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if ( n > 0 )
    {
        // 已建立链接的用户，有可读事件发生了，调用用户传入的回调操作
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n==0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead \n");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) // 没有可读数据了，已发送完成
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    // 唤醒loop_对应的thread线程，执行回调
                    // 实际上读写操作就是在当前subLoop上执行的，但queueInLoop也没错
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
                }
            }
            if (state_ == kDisconnecting)
            {
                shutdownInLoop();   // 在当前Loop中关闭TcpConnection
            }
        }
        else
        {
            LOG_ERROR("TcpConnection handleWrite \n");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing", channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("fd=%d state=%d \n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableALL();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);   // 执行关闭的回调
    closeCallback_(connPtr);    // 关闭链接的回调，执行的是TcpServer::removeConnection回调
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SDCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError \n");
}

void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, 
                                        buf.c_str(), buf.size()));
        }
    }
}


// 发送数据，应用写的快，内核发送数据慢，所以需要把待发送数据送入缓冲区
// 通过设置水位回到，防止发送太快
void TcpConnection::sendInLoop(const void* data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    // 之前调用过该connection的shutdown，不能再进行发送
    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing");
        return;
    }

    // 表示channel第一次开始写数据，而且缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote > 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                // 既然在这数据已全部发送完成，就不用再给channel设置epollout事件
                loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
            }
        }
        else    // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop \n");
                if (errno == EPIPE || errno == ECONNRESET)  // sigpipe和reset
                {
                    faultError = true;
                }
            }
        }
    }

    // 说明还有数据没发送出去，所以需要把剩余的保存到缓冲区中
    // 然后给channel注册epollout事件，poller通过水平触发，发现tcp的发送缓冲区有空间
    // 则会通知sock-channel调用writeCallback_方法，也就是上面的handleWrite方法
    // 直到所有的数据发送完成
    if (!faultError && remaining > 0)
    {
        // 剩余发送缓冲区的数据长度
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_
                && oldLen < highWaterMark_
                && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(),
                                oldLen + remaining));
        }
        outputBuffer_.append((char*)data + nwrote, remaining);
        if (!channel_->isWriting())
        {
            // 这里一定要注册channel的写事件，否则Poller不会给channel通知epollout
            channel_->enableWriting();  
        }
    }
}

// 在shutdown的过程中，数据可能还在发送过程中
void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting()) // 说明outpubuffer的数据已经全部发送完成
    {
        socket_->shutdownWrite();   // 关闭写，触发EPOLLHUP
    }
}

// 链接建立
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();  // 向Poller注册channel的epollin事件
    
    // 新连接建立，执行回调
    connectionCallback_(shared_from_this());

}

// 链接销毁
void TcpConnection::connectDestroyed()
{
    if (state_ = kConnected)
    {
        setState(kDisconnected);
        channel_->disableALL();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}