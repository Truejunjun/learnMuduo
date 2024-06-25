#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class EventLoop;
class Channel;
class Socket;

///
/// TcpServer → Acceptor → 有一个新用户链接，通过accept函数拿到connfd
/// 封装得到TcpConnection，设置相应回调 → Channel → Poller → Channel回调操作
///


class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,
                    const std::string &name,
                    int sockfd,
                    const InetAddress& localAddr_,
                    const InetAddress& peerAddr_);

    ~TcpConnection();

    EventLoop* getLoop() cosnt { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress()   { return localAddr_; }
    const InetAddress& peerAddress()   { return peerAddr_; }

    bool connected() cosnt { return state_ == kConnected; }

    // 回调设置操作，TcpServer中也有具体的操作
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb;}
    void setMessageCallback(const MessageCallback &cb)    { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)    { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)  
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark;}
    void setCloseCallback(const CloseCallback& cb)  { closeCallback_ = cb;}

    // 链接建立
    void connectEstablished();
    // 链接销毁
    void connectDestroyed();

private:
    enum StateE {kDisconnected, kConnecting, kConnected, kDisconnecting};
    void setState(StateE state) { state_ = state };
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    // 考虑到使用者可能没有实现buffer类，没有相应操作，所以可以考虑直接输出string类型
    void send(const std::string &buf);
    void sendInLoop(const void* message, size_t len);
    void shutdown();
    void shutdownInLoop();

    EventLoop *loop_;   // 这里肯定不是mainLoop，因为是通过TcpConnection都是在subLoop中
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    // 这里与Acceptor类似，Acceptor存在于MainLoop， TcpConnection存在于SubLoop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_; // 有新连接时的回调
    MessageCallback messageCallback_;   // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;   // 消息发送完成后的回调
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;

    Buffer inputBuffer_;    // 接受数据的缓冲区
    Buffer outputBuffer_;   // 发送数据的缓冲区
};