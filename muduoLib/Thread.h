#pragma once

#include "noncopyable.h"
#include <functional>
#include <thread>   // 不能直接使用std::thread thread_，一使用就会创建线程开始运行
#include <memory>   // 智能指针 
#include <string>
#include <atomic>

class Thread : noncopyable
{
public:
    using ThreadFunc = std::fucntion<void()>;

    explicit Thread(ThreadFunc, const std::string &name = string());    // 参数的默认值只出现一次
    ~Thread();

    void start();
    void join();

    // 查询函数
    bool started() const { return started_;}
    pid_t tid() const { return tid_;}   // 相当于top查看的tid线程号，并不是真正的线程号
    const std::string& name() const { return name_;}
    
    static int numCreated() { return numCreated_;}

private:
    void setDefaultName();
    
    bool started_;
    bool joined_;

    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;

    static std::atomic_int numCreated_;
};
