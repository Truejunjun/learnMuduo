#include "channel.h"
#include "EventLoop.h"
#include "logger.h"
#include <sys/epoll.h>

// 定义成员变量的时候不需要使用static
// 只使用EPoll
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;	// 具体可以见头文件，EPOLLIN=0x001，EPOLLPRI=0x002
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *Loop, int fd)
	: loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel() 
{}

// 什么时候会被调用？？
void Channel::tie(const std::shared_ptr<void>&obj)
{
	tie_ = obj;
	tied_ = true;
}

// 当改变了Channel所表示fd的events事件后，update负责在poller中更改fd对应的事件epoll_ctl
// Channel和Poller可以理解为同等级的不同模块，之间要保证同步
void Channel::update()
{
	// 通过所属的EventLoop，调用Poller的相应方法，注册fd的events事件
	loop_->updateChannel(this);
    
}

// 在Channel所属的EventLoop中，把当前的Channel删掉
void Channel::remove()
{
	loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
	if (tie_){
		std::shared_ptr<void> guard = tie_.lock();	// 提升为强智能指针
		if (guard){
			handleEventWithGuard(receiveTime);
		}
	}
	else{
		handleEventWithGuard(receiveTime);
	}
}

// 执行对应的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
	// 先判断是否异常
	if ((revents_ & EPOOLLHUP) && !(revents_ & EPOLLIN)){
		if (closeCallback_){
			closeCallback_();
		}
	}
	
	// 处理错误事件
	if (revents_ & EPOLLERR){
        if (errorCallback_){
			errorCallback_();
		}
	}
	
	// 处理读事件
	if (revents_ & (EPOLLIN | EPOLLPRI)){
        if (readCallback_){
			readCallback_(receiveTime);
		}
	}
	
	// 处理写事件
	if (revents_ & EPOLLOUT){
        if (writeCallback_){
			writeCallback_();
		}
	}
}