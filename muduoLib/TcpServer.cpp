#include "TcpServer.h"
#include "Logger.h"
#include <strings.h>
#include "TcpConnection.h"

#include <functional>

static EventLoop* checkLoopNotNull(EventLoop *loop)
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
    nextConnId_(1),
    started_(0) // 保证整个start只被启动一次
{
    // 两个占位符，connfd和peerAddr,当有新用户连接时就会执行回调函数
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
                                        std::placeholders::_1, std::placehoders::_2));
    
}

TcpServer::~TcpServer()
{
    for (auto& item : connections_)
    {
        // 用局部shared_ptr智能指针对象，出右括号，可以自动释放new出来的TcpConnection对象资源
        TcpConnectionPtr conn(item.second); 
        item.second.reset();

        // 通过局部临时变量销毁链接
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
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

// 有一个新的客户端的链接，acceptor会执行这个回调，只会有mainLoop在处理
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    // 轮询算法，选择一个subLoop，并组装其名字
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s\n",
        name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    
    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if (::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr \n ");
    }

    InetAddress localAddr(local);

    // 根据连接成功的sockfd，创建TcpConnection链接对象
    // 有了sockfd可以创建socket和channel
    TcpConnectionPtr conn(new TcpConnection(
        ioLoop, connName, sockfd, localAddr, peerAddr
    ))

    connections_[connName] = conn;
    // 下面的回调都是用户设置给TcpServer，然后传递给TcpConnection
    // 再由TcpConnection传递给Channel，再传递让Poller通知Channel来执行操作
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // 设置了如何关闭链接的回调
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this
                                        std::placeholders::_1));
    // 直接调用
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(&TcpServer::remoceConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - new connection [%s] from %s\n",
                    name_.c_str(), conn->name().c_str());
    
    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}