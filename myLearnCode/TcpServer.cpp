#include "TcpServer.h"
#include "Logger.h"

#include <functional>

inline EventLoop* checkLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("mainLoop is null");
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress &listenAddr, const std::string nameArg, Option option)
    : loop_(checkLoopNotNull(loop)), ipPort_(listenAddr.toIpPort()), name_(nameArg)
    acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)), 
    threadPool_(new EventLoopThreadPool(loop, name_)),  // 默认只有主线程
    connectionCallback_(),  // 还缺默认调用，后面写？？？
    messageCallback_(),
    nextConnId_(1)
{
    // 两个占位符，connfd和peerAddr,当有新用户连接时就会执行回调函数
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
                                        std::placeholders::_1, std::placehoders::_2));
    
}

TcpServer::~TcpServer()
{

}

// 设置底层subloop的个数
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

// 开启监听，也就是开启mainLoop中的Acceptor的loop监听， loop.loop()
void TcpServer::start()
{
    // 防止一个TcpServer对象被启动多次
    if (started_++ == 0)
    {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}


void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{

}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{

}

void TcpServer::remoceConnectionInLoop(const TcpConnectionPtr &conn)
{

}