#pragma once
#include "noncopyable.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::functional<void(EventLoop*)>

    EventLoopThreadPool(EventLoop* baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads)   { numCreated_ = numThreads; }
    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    // 如果工作在多线程中，baseLoop默认以轮询的方式分配channel给subloop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();
    bool started() const { return started_; }
    std::string name() const { return name_; }

private:
    EvnetLoop *baseLoop_;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    
    std::vector<std::unique_ptr<EventLoopThread>> threads_; // 包含了所有的线程
    std::vector<EventLoop*> loops_;     // 事件EventLoop中的指针
};