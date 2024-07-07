#include "chatServer.hpp"
#include "chatService.hpp"
#include <iostream>
#include <signal.h>

using namespace std;

// 处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int)
{
	ChatService::instance()->reset();
	exit(0);
}


int main()
{
	signal(SIGINT, resetHandler);

	EventLoop loop;	// 相当于epoll
	InetAddress addr("Linux IP Addr", 6000);
	ChatServer server(loop, addr, "muduo_server");
	
	server.start();	// 完成的操作有listenfd opell_ctl->epoll，将需要监听的套接字通过ctl添加到epoll上
	loop.loop();	// epoll_wait以阻塞的方式等待新用户的链接，已连接用户的读写操作
	
	return 0;
}

/*
* 测试代码见下

# 启动服务之后，可以通过以下测试
telnet IPAddr PortNum	# 启动服务

# 然后另起一个shell，输入命令测试
hello world!	# 因为是回声服务器，所以会返回输入

*/