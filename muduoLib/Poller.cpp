#include "Poller.h"
#include "Channel.h"	// 使用到了fd()方法

Poller::Poller(EventLoop *loop)
	: ownerLoop_(loop)
{
}

bool Poller::hasChannel(Channel *channel) const
{
	auto it = channels_.find(channel->fd());
	// 判断其寻找是否落空且找到的map的value就是当前channel
	return it != channels_.end() && it->second == channel;
}