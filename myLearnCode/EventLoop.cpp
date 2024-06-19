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

    }
}
