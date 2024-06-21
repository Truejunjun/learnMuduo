#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(cosnt ThreadInitCallback &cb, const std::string &name)
    : loop_(nullptr), exiting_(false), thread_(std::bind(&EventLoopThread::threadFunc, this), name),
    mutex_(), cond_(), callback_(cb)
{
    
}


EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_ -> quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    thread_.start();    // 开启了底层的一个新线程，也就是运行func(),也就是threadFunc()

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_ == nullptr)
        {
            cond_.wait(lock);   // 等待在互斥锁上
        }
        loop = loop_;
    }
    return loop;
}

// 是在单独的新线程中使用
void EventLoopThread::threadFunc()
{
    EventLoop loop; // 创建一个独立的eventloop，与线程一一对应

    if (callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();    // 开启底层的poller开始监听，一般都会一直停在这一句，一直保持工作
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}