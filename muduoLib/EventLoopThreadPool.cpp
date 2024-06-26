#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include "memory.h"


EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop), name_(nameArg), started_(false),
    numThreads_(0), next_(0)
{}

// 不需要手动清理，因为见EventLoopThread::threadFunc()，Loop都是在线程上，
// 也就是在堆栈的情况下创建的，当loop结束会自动析构
EventLoopThreadPool::~EventLoopThreadPool()
{}


void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    started_ = true;

    for (int i = 0; i < numThreads_; ++i)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i); // 作为线程的名字
        EventLoopThread *t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));    // 智能指针会自动释放资源
        loops_.push_back(t->startLoop());   // 底层创建线程，绑定一个新的EventLoop，并返回该Loop的地址
    }

    // 如果从未设置过线程数量，则跳过上面的循环;且当前只有一个线程运行着baseloop
    if(numThreads_ == 0 && cb)
    {
        cb(baseLoop_);
    }
}

// 如果工作在多线程中，baseLoop默认以轮询的方式分配channel给subloop
EventLoop* EventLoopThreadPool::getNextLoop()
{
    EventLoop *loop = baseLoop_;

    if (!loops_.empty())    // 通过轮询获取下一个事件的loop
    {
        loop = loops_[next_++];
        if (next_ >= loops_.size())
        {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    if (loops_.empty())
    {
        return std::vetor<EventLoop*>(1, baseLoop_);
    }
    else
    {
        return loops_;
    }
}