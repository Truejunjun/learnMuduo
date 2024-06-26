#pragma once

#include "noncopyable.h"
#include "Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>

class EventLoop;    // 只用到类，需要声明，且没用到里面的成员

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::functional<void(EventLoop*)>
    EventLoopThread(cosnt ThreadInitCallback &cb = ThreadInitCallback(),
        const std::string &name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop();

private:
    void threadFunc();  // 线程函数，创建Loop

    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};