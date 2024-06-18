#include <muduo/net/TcpServer>
#include <muduo/net/EventLoop>
#include <functional>	// 绑定器在这
#include <iostream>
#include <string>

using namespace muduo;
using namespace muduo::net;
using namespace placehodlers;

/*基于muduo网络库开发服务器程序
1. 组合TecpServer对象
2. 创建EventLoop事件循环对象的指针
3. 明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4. 在当前服务器类的构造函数中，注册处理链接断开的回调函数和处理读写的回调函数
5. 设置合适的服务端线程数量，muduo库会自动分配IO和worker线程
*/

class ChatServer{
public:
	// 手动编写构造函数
	ChatServer(EventLoop *loop,	// 事件循环（也就是Reactor）
				const InetAddress &listenAddr,	// IP+Port
				const string &nameArg)	// 服务器的名字
        : _server(loop, listenAddr, nameArg), _loop(loop)
        {
        	// 给服务器注册用户链接的创建和断开回调，由于返回值不同，需要设定绑定器
        	// _1是参数占位符，显示有一个参数且为第一个
        	_server.onConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));	
        	
        	// 给服务器注册用户读写事件回调
        	_server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
        	
        	// 设置服务器的线程数量
        	_server.setThreadNum(4);	// 1个IO线程，3个工作线程
        }
        
        void start(){
        	_server.start();
        }
private:
	// 专门处理用户的链接创建和断开，函数来自TcpServer库
	void onConnection(const TcpConnectionPtr &conn)
	{
		if (conn->connected()){
			cout << conn->peerAddress().toIpPort() << "->" << 
			conn->localAddress().toIpPort() << "state:Online" << endl;
		}
		else{
			cout << ...... << "state:Offline" << endl;
			conn->shutdown();
			// _loop->quit();
		}
	
	}
	
	// 专门处理用户的读写事件
	void onMessage(const TcpConnectionPtr &conn，// 链接
    				Buffer *buffer,
    				Timestamp time)	//	接收到数据的时间信息
    {
		string buf = buffer->retrieveAllAsString();
		cout << "recv data:" << buf << "time cost:" << time.toString() << endl;
		conn->send(buf);
	}
	TcpServer _server;	// 第一步
	EventLoop *_loop;	// 第二步
};


int main()
{
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