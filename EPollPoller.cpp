#include "EPollPoller.h"
#include "logger.h"
#include <errno.h>

const int kNew = -1;	// 状态值，表示Channel还未添加到Poller中
const int kAdded = 1;
const int kDeleted = 2;


EPollPoller::EPollPoller(EventLoop *loop)	// Poller是从基类继承而来
	: Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize)	// events_是EventList类型，也就是vector<epoll_event>
{
	if (epollfd_ < 0)
	{
		LOG_FATAL("epoll_create error:%D \n", errno);	// 报错并停止
	}
}

EPollPoller::~EPollPoller()
{
	::close(epollfd_);
}


TimeStamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
	
}

void EPollPoller::updateChannel(Channel *channel)
{

}

void EPollPoller::removeChannel(Channel *channel)
{

}

// 填写活跃的链接
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{

}

// 更新Channel
void EPollPoller::update(int operation, Channel *channel)
{

}