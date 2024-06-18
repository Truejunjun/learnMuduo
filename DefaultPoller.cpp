// 该文件是一个公共文件
#include "Poller.h"
#include <stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop *loop)
{
	// 获取环境变量，一般muduo都是默认使用Epoll
	if (::getenv("MUDUO_USE_POLL"))
	{
		return nullptr;	// 实现poll实例
	}
	else
	{
		return nullptr;	// 实现Epoll实例
	}
}