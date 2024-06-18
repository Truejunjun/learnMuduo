#pragma once
#include "Poller.h"
#include <vector>
#include <sys/epoll.h>
#include "timeStamp.h"

class Channel;	// 因为只用到指针，所以声明即可

class EPollPoller : public Poller
{
public:
	EPollPoller(EventLoop *loop);
	~EPollPoller() override;
	
	// 重写Poller基类的抽象方法
	// 相当于epoll_wait
	Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
	// 相当于epoll_ctl
	void updateChannel(Channel *channel) override;
	void removeChannel(Channel *channel) override;
	
private:
	static const int kInitEventListSize = 16;
	
	// 填写活跃的链接
	void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
	// 更新Channel
	void update(int operation, Channel *channel);

	using EventList = std::vector<epoll_event>;	// epoll_event是一个结构体，存发生事件events
	
	int epollfd_;
	EventList events_;
}