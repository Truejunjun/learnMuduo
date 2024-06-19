#pragma once

#include "noncopyable.h" // 该文件作为基类，会使用到其中的函数或变量，所以采用include，而不是声明
#include <functional>
#include <vector>
#include <atomic>
#include "Timestamp.h"
#include <memory>   // 智能指针的头文件
#include <mutex> // 需要使用到锁
#include "currentThread.h"

class Channel;
class Poller;

class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_;}

    void runInLoop(Functor cb); // 在当前loop中执行cb
    void queueInLoop(Functor cb);   // 把cb放入队列中，唤醒loop所在的线程，执行cb

    // 唤醒loop所在的线程
    void wakeup();

    //  EventLoop的方法，具体调用与Poller
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    // 判断EventLoop是否在自己的线程中
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }; 
private:
    void handelRead();
    void doPendingFunctors();


    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;  // 原子操作的布尔值，通过CAS实现
    std::atomic_bool quit_;  // 标志退出loop循环

    const pid_t threadId_;  // 记录当前loop所在的线程id(EventLoop通过线程id来独立执行各自的loop操作)

    Timestamp pollReturnTime_;  // POller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;

    // 可以多去了解eventfd()函数muduo库使用的就是该函数
    // 当mainLoop获取一个新用户的channel，通过轮询算法选择一个subLoop，通过该成员唤醒subLoop处理channel
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;
    Channel* currentActiveChannel_; // 可以不用

    std::atomic_bool callingPendingFunctors_;   //  标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;  // 存储所有loop需要执行的回调操作
    std::mutex mutex_;  // 互斥锁，用来保护上面vector容器的线程安全操作
}