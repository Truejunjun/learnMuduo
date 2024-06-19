#include "EventLoop.h"
#include "logger.h"
#include "Poller.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>

// 防止一个线程创建多个EventLoop，加上__thread
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd，用来notify唤醒subReactor处理新来的channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd err:%D \n", errno);
    }
    return evtfd;
}

// 构造函数，并填充成员变量
EventLoop::EventLoop()
    : looping_(false), quit_(false), callingPendingFunctors_(false), 
    threadId_(CurrentThread::tid()), poller_(Poller::newDefaultPoller(this)),
    wakeupFd_(createEventfd()), wakeupChannel_(new Channel(this, wakeupFd_)),
    currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", this, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型和发生事件后的回调操作
    wakeupChannel_ -> setReadCallback(std::bind(&EventLoop::handelRead, this));
    // 每一个eventLoop都将监听wakeupChannel的EPOLLIN读事件
    wakeupChannel_ -> enableReading();
}


// 由于其他成员是智能指针，所以调用析构函数的时候会自动del
EventLoop::~EventLoop()
{
    wakeupChannel_ -> disableAll(); // 禁止所有的感兴趣事件
    wakeupChannel_ -> remove(); // 从EventLoop中删除该channel，具体的实现在EventLoop类中
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}


void EventLoop::handelRead()
{
    uint64_t one = 1;
    // ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("");
    }
}