#include "Acceptor.h"
#include "Logger.h"

#include <sys/types.h>
#include <sys/socket.h> // bind需要这两个头文件
#include <errno.h>
#include <unistd.h>

static int creatNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err :%d \n", __FILE__, __FUNCITON__, __LINE__, errno);
    }
}

Acceptor::Acceptor(EvnetLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop), acceptSocket_(creatNonblocking()),   // 创建套接字
    acceptChannel_(loop, acceptSocket_.fd()), listenning_(false),
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReuseport(true);
    acceptSocket_.bindAddress(listenAddr);  // 绑定套接字
    // TcpServer::start() Acceptor.listen 监听新用户的连接，要执行一个回调则
    // 则先创建一个confd，再打包成channel，再下发给subloop
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableALL();
    acceptChannel_.remove();
}


void Acceptor:listen()
{
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading(); // 将channel注册进Poller
    // 首先开始新用户的监听，通过使能将channel丢入Poller，当成功监听到，则操作回调函数handleRead
    // 通过回调函数得到confd和Addr，然后执行新连接的回调操作
}


// listenfd有事件发生了，就是有新用户连接了
void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int confd = acceptSocket_.accept(&peerAddr);    // 返回了通信用的fd
    if (confd > 0)
    {
        if (NewConnectionCallback_)
        {
            NewConnectionCallback_(confd, peerAddr);    // 轮询找到subloop，唤醒，分发当前新客户端的channel
        }
        else
        {
            ::close(confd); // 如果回调函数都没有证明无法服务，直接关闭
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err :%d \n", __FILE__, __FUNCITON__, __LINE__, errno);
        if (errno == EMFILE)
        {
            LOG_ERROR("socketfd reach limit error");  // 可能是当前进程超过了文件描述符的上限
        }
    }
}