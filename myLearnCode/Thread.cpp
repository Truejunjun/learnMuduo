#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

std::atomic_int Thread::numCreated_ = 0;    // static成员变量，在类外定义

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false), joined_(false), tid_(0), func_(std::move(func)), name_(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    // 如果线程正在运行，并且如果是joined，主线程结束其也会结束，如果不是joined就需要分离操作
    if (started_ && !joined_)   
    {
        thread_ -> detach();    // thread类提供了设置分离线程的方法
    }
}


// 一个thread对象，记录的就是新线程的详细信息
void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    // 开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&]()
    {
        tid_ = CurrentThread::tid();
        sem_post(&sem); // 在获取的tid之后，给信号资源+1
        func_();    // 开启一个新线程，专门执行该线程函数
    }))

    // 必须等待上面的操作获取线程号tid
    sem_wait(&sem);
}


void Thread::join()
{
    joined_ = true;
    thread_ -> join();
}


void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);  // 定义名字
        name_ = buf;
    }
}