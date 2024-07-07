#include "chatServer.hpp"
#include "chatService.hpp"
#include "json.hpp"

#include <functional>	// 绑定器在这
#include <iostream>
#include <string>

using namespace muduo;
using namespace muduo::net;
using namespace placehodlers;
using json = nlohman::json;

/*基于muduo网络库开发服务器程序
1. 组合TecpServer对象
2. 创建EventLoop事件循环对象的指针
3. 明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4. 在当前服务器类的构造函数中，注册处理链接断开的回调函数和处理读写的回调函数
5. 设置合适的服务端线程数量，muduo库会自动分配IO和worker线程
*/


// 手动编写构造函数
ChatServer::ChatServer(EventLoop *loop,	// 事件循环（也就是Reactor）
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
    


void ChatServer::start(){
    _server.start();
}



// 专门处理用户的链接创建和断开，函数来自TcpServer库
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected()){
        cout << conn->peerAddress().toIpPort() << "->" << 
        conn->localAddress().toIpPort() << "state:Online" << endl;
    }
    else{
        cout << ...... << "state:Offline" << endl;
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
        // _loop->quit();
    }

}



// 专门处理用户的读写事件
void ChatServer::onMessage(const TcpConnectionPtr &conn,// 链接
                Buffer *buffer,
                Timestamp time)	//	接收到数据的时间信息
{
    string buf = buffer->retrieveAllAsString(); // 从缓冲区中把数据转成string
    // 数据反序列化
    json js = json::parse(buf);

    // 通过js["msgid"] 获取业务处理器
    // 达成的目的---完全解耦网络模块的代码和业务模块的代码
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    msgHandler(conn, js, time);

    // 回声服务器测试
    // cout << "recv data:" << buf << "time cost:" << time.toString() << endl;
    // conn->send(buf);
}