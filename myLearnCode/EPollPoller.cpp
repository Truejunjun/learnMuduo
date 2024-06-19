#include "EPollPoller.h"
#include "logger.h"
#include "channel.h"	// 因为要使用到channel中的具体的成员变量
#include <errno.h>
#include <unistd.h>		// 使用到close函数
#include <strings.h>	// 使用memZero的替换函数bzero

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



// 实际上就对应着Reactor中的epoll_wait()
TimeStamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
	// 实际上应该使用LOG_DEBUG，因为poll需要频繁地调用，所以不要使用log占用效率
	LOG_DEBUG("func=%s => fd total count:%lu \n", __FUNCTION__, channels_.size());

	int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int> events_.size(), timeoutMs);	// 需要拿数组的起始地址
	int saveErrno = errno;	// 因为该函数会被多次调用，所以使用局部变量将其存起来
	TimeStamp now(TimeStamp::now());


	if (numEvents > 0)
	{
		LOG_DEBUG("%d events happened \n",numEvents);
		fillActiveChannels(numEvents, activeChannels);	// 将发生的event更新

		// 接下来判断并完成扩容
		if (numEvents == events_.size())
		{
			events_.resize(events_.size() * 2);	// 两倍扩容
		}
	}
	else if (numEvents == 0)
	{
		LOG_DEBUG("%s timeout \n", __FUNCTION__);
	}
	else
	{
		if (saveErrno != EINTR)	// 如果保存的errno不等于外部中断
		{
			errno = saveErrno;	// 因为日志获取error是通过全局的变量，所以需要更新一下回去
			LOG_ERROR << "EPollPoller::Poll() Error."	
		}
	}
	return now;	// 返回发生的时间点
}




/*	关键变量的逻辑树，其中ChannelList的数量 ≥ ChannelMap的数量

				EventLoop
	ChannelList				Poller
					ChannelMap <fd, channel*>

*/
void EPollPoller::updateChannel(Channel *channel)
{
	const int index = channel->index();
	LOG_INFO("fd=%d events=%d index=%d \n", channel->fd(), channel->events(), index);

	if (index == kNew || index == kDeleted)
	{
		if (index == kNew)
		{
			int fd = channel->fd();
			channels_[fd] = channel;	// 先在Poller中的哈希表注册新的事件
		}

		channel->set_index(kAdded);		// 修改当前channel的状态
		update(EPOLL_CTL_ADD, channel);	// 将channel进行注册
	}
	else	// 说明当前channel已经注册了
	{
		int fd = channel->fd();
		if (channel->isNoneEvent())
		{
			update(EPOLL_CTL_DEL, channel);
			channel->set_index(kDeleted);
		}
		else
		{
			update(EPOLL_CTL_MOD, channel);
		}
	}
}


void EPollPoller::removeChannel(Channel *channel)
{
	// 如果是del状态只用在channel_map上删除，如果是added状态则还要在epoll中删除
	int fd = channel->fd();
	int index = channel->index();
	channels_.erase(fd);

	if (index == kAdded)
	{
		update(EPOLL_CTL_DEL, channel);
	}
	channel->set_index(kNew);
}


// 填写活跃的链接
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
	for (int i=0; i < numEvents; ++i)
	{
		Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
		channel->set_revents(events_[i].events)
		activeChannels->push_back(channel);	// EventLoop拿到了从Poller返回的所有发生事件的列表
	}
}


// 更新Channel，完成的是Add，Mod，Del的操作，也就是epoll_ctl
// operation就是EPOLL_CTL_XXX
void EPollPoller::update(int operation, Channel *channel)
{
	struct epoll_event event;
	// mmZero(&event, 0, sizeof event);	// 创建event
	bzero(&event, sizeof event);		// 替换掉memZero
	event.events = channel->events();
	event.data.ptr = channel;	// 指针指向channel
	int fd = channel->fd();
	event.data.fd = fd;

	if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)	// 调用自带的修改函数
	{
		if (operation == EPOLL_CTL_DEL)
		{
			LOG_ERROR("epoll_ctl del error:%d\n", errno);
		}
		else
		{
			LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
		}
	}
}