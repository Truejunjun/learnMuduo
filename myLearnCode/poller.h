#pragma once

#include "nocopyable.h"
#include "TimeStamp.h"
#include <vector>
#include <unordered_map>

class Channel;		// 因为只是使用到指针成员变量，指针大小固定，不随类型改变，所以声明即可
class EventLoop;	// 也是指针类型

// muduo库中多路事件分发器的核心IO复用模块
class Poller : nocopyable
{
public:
	using ChannelList = std::vector<Channel*>;
	
	Poller(EventLoop *loop);
	virtual ~Poller() = default;
	
	// 给所有IO复用保留统一的接口
	virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
	virtual void updateChannel(Channel *channel) = 0;
	virtual void removeChannel(Channel *channel) = 0;
	
	// 判断参数channel是否在当前Poller中 
	bool hasChannel(Channel *channel) const;
	
	// EventLoop可以通过该接口获取默认的IO复用的具体实现，具体指Epoll、Poll或Select
	// *****虽然头文件中包含该函数，但是在cpp中并不完成其实现，因为Poller是基类，实现newDefaultPoller需要调用Epoll和Poll，所以需要include派生类，这种实现方式不好，一般是派生类引用基类。所以源码中单独创立了一个文件去实现这个函数，调用了两个派生，而不是在Poller这个抽象基类中实现
	static Poller* newDefaultPoller(EventLoop *loop);
	
protected:
	// map的key写的是sockfd，value写的是所属的channel类型
	using ChannelMap = std::unordered_map<int, Channel*>;
	ChannelMap channels_;
	
private:	
	EventLoop *ownerLoop_; // 定义Poller所属的EvnetLoop
}