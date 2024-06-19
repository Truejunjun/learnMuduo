#pragma once

#include "noncopyable.h"
#include "timestamp.h"	// 因为使用到形参，需要头文件定义以下变量的大小
#include <functional>
#include <memory>

class EventLoop;	// 只是做类的声明，不用在头文件暴露过多的信息

class Channel: noncopyable
{
public:
	//typedef std::function<void()> EventCallback
	using EventCallback = std::function<void()>;
	using ReadEventCallback = std::function<void(Timestamp)>;
	
	Channel(EventLoop *loop, int fd);
	~Channel();
	
	// fd得到poller通知之后，处理事件的
	void handleEvent(Timestamp receiveTime);
	
	// 设置回调函数
	void setReadCallback(ReadEventCallback cb){readCallback_ = std::move(cb)};
	void setWriteCallback(EventCallback cb){writeCallback_ = std::move(cb)};
	void setCloseCallback(EventCallback cb){closeCallback_ = std::move(cb)};
	void setErrorCallback(EventCallback cb){errorCallback_ = std::move(cb)};
	
	// 防止当channel被手动remove掉，channel还在执行回调操作
	void tie(const std::shared_ptr<void>&);
	
	// 查询和设置函数
	int fd() const {return fd_;}
	int event() const {return events_;}
	int set_revents(int revt) const {revents_ = revt;}	// Poller监听事件并激活Channel
	bool isNoneEvent() const {return events_ == kNoneEvent;}	// 判断是否注册事件
	
	// 设置fd相应的事件状态
	void enableReading() {events_ |= kReadEvent; update(); }
	void disableReading() {evnets_ &= ~kReadEvent; update();}
	void enableWriting() {events_ |= kWriteEvent; update(); }
	void disableWriting() {evnets_ &= ~kWriteEvent; update();}	
	void disableALL() {evnets_ = kNoneEvent; update();}
	
	// 返回fd当前的事件状态
	bool isNoneEvent() const {return events_ == kNoneEvent; }
	bool isReading() const {return events_ == kReadEvent; }
	bool isWriting() const {return events_ == kWriteEvent; }
	
	int index(){return index_;}
	void set_index(int idx) {index_ = idx;}
	
	// one loop per thread
	EventLoop* ownerLoop() {return loop_;}
	void remove();
    
private:
	static const int KNoneEvent;	// 感兴趣事件状态，这三个只是标识
	static const int KEeadEvent;
	static const int KWriteEvent;
	
	EventLoop *loop_;	// 事件循环
	const int fd_;		// fd, Poller监听的对象
	int events_;		// 注册fd感兴趣事件
	int revents_;		// Poller返回的具体发生的事件
	int index_;
	
	std::weak_ptr<void> tie_;
	bool tied_;
	
	// 因为channel通道里面能获得fd里确切发生的事件revents，所以负责具体的回调操作
	// 保存事件到来时的回调函数。EventCallback是std::function类型，可以粗暴看作是函数指针
	ReadEventCallback readCallback_;
	EventCallback writeCallback_;
	EventCallback closeCallback_;
	EventCallback errorCallback_;
	
	void update();
	void handleEventWithGuard(Timestamp receiveTime);
}