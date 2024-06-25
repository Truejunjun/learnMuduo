#pragma once

// 防止用户使用时要自己添加过多头文件
#include "EventLoop.h"
#include "Acceptor.h"
#include "noncopyable.h"
#include "InetAddress.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    // 枚举
    enum Option
    {
        kNoReusePort;
        kReusePort;
    };

    TcpServer(EventLoop* loop, const InetAddress &listenAddr, const std::string nameArg, Option option = kNoReusePort);
    ~TcpServer();

    // 设置回调
    void setThreadInitCallback(const ThreadInitCallback &cb)    { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb)    { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb)    { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)    { writeCompleteCallback_ = cb; }

    // 设置底层subloop的个数
    void setThreadNum(int numThreads);

    // 开启监听，也就是开启mainLoop中的Acceptor的loop监听
    void start();

private:
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void remoceConnectionInLoop(const TcpConnectionPtr &conn);


    EventLoop* loop_;   // 也就是baseloop
    
    cosnt std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_;    // 运行在mainLoop中，监听新连接事件

    std::shared_ptr<EventLoopThreadPool> threadPool_;   // one loop per thread

    ConnectionCallback connectionCallback_; // 有新连接时的回调
    MessageCallback messageCallback_;   // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;   // 消息发送完成后的回调
    CloseCallback closeCallback_;

    ThreadInitCallback threadInitCallback_; // loop线程初始化的回调
    
    std::atomic_int started_;
    int nextConnId_;
    ConnectionMap connections_;
};