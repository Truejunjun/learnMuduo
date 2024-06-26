#include "EventLoop.h"
#include "logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>   // 锁
#include <mutex>

// 防止一个线程创建多个EventLoop，加上__thread
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd，用来notify唤醒subReactor处理新来的channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);   // 通过EFD_NONBLOCK设置为非阻塞状态，一但非0就代表发生可读事件
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


void Event::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);
    while(!quit_)
    {
        // 先清空旧的激活事件
        activeChannels_.clear();
        // 监听两类fd，一种是client的fd，一种是wakeupfd
        // 这一句是核心监听代码，会发生阻塞
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_); // 监听并获取事件，存入感兴趣channelList
        for (Channel *channel : activeChannels_)
        {
            // Poller监听哪些channel发生了事件，并上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }

        
        // 执行当前EventLoop事件循环需要处理的回调操作
        /*
        * IO线程 mainLoop accept fd ← channel subloop
        * mainLoop 事先注册一个回调cb（需要subloop来执行） wakeup subloop后，执行下面的方法
        * 执行之前mainloop注册的cb回调操作，vector<Functors>
        */
        doPendingFunctors();
    }

    LOG_INFO("EventLoop stop looping %p \n", this);
    looping_ = false;
}


// 退出事件循环 
// 1.Loop在自己的线程中调用quit，那当然已完成了阻塞自行执行
// 2.在非Loop的线程中调用loop的quit，例如在工作线程中调用IO线程的quit
void EventLoop::quit()
{
    quit_ = true;

    if (!isInLoopThread())    // 判断当前线程是否等于主线程mainloop的threadId
    {
        wakeup();   // 从阻塞中醒来，重新跑一遍loop()，需要判断失败跳出while循环
    }
}


void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread()) // 在当前的线程loop中，执行cb
    {
        cb();
    }
    else    // 在非当前线程中执行cb，例如在subloop1中想执行subloop2的cb；所以需要唤醒
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::uniqu_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒相应的需要执行上面回调操作的loop线程
    // 或上第二个条件：当前loop正在执行回调，但是loop又来了新的回调操作，防止一直阻塞不操作
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();   // 唤醒loop所在线程
    }
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


// 向wakeupFd_写一个数据，wakeupChannel就会发生读事件，当前loop线程就会被唤醒 
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true; // 开始执行回调

    {
        // 通过交换的方式，直接将待执行的操作存储到局部的functors中，使原来的线程的vector制空
        // 不影响原线程的操作，减少滞后延迟
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors)
    {
        functor();
    }
    callingPendingFunctors_ = false;
}


// channel使用update()后，无法直接访问Poller，所以通过父类EventLoop调用EPollPoller中的实现
void EventLoop::updateChannel(Channel *channel){
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel){
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel){
    return poller_->hasChannel(channel);
}