# learnMuduo 🌏
该工程主要记录本人学习Muduo项目过程中的核心代码，核心代码已完成，但是可能存在Bug，后续编译时会解决。🐖

# 快速使用 🔧
可以通过执行`autobuild.sh`直接完成代码的编译。


# muduo库梳理 📚
以下是muduo库核心代码的关系梳理，包含成员变量和函数调用，持续更新中。🚀
## 成员变量
***

<font color=blue>**EventLoop**</font>() → Reactor

​	ChannelList **activeChannels**_ (vector<*Channel> 类型)     
​	int **wakeupFd_**     
​	unique_ptr<Channel> **wakeupChannel_**     
​	unique_ptr<Poller> **poller_**     

> 每一个Loop都具有wakeupFd，可以通过写入8字节无用数据唤醒
> EventLoop涵盖有下两个大类，分别是Channel和Poller

***

<font color=blue>**Channel**</font>(EventLoop *loop, int fd)

​	EventLoop* **loop_**     
​	int **fd_**     
​	int **events_**     
​	int **revents_**     
​	std::function<void()/...> **xxxxCallback_**     

> 实际上只存在两种Channel, 一种是listenfd→acceptorChannel, 一种是connfd→connectionChannel

***

<font color=blue>**Poller **</font>(EventLoop *loop)  →  事件分发器Demultiplex

​	EventLoop* **ownerloop_**     
​	unordered_map<int fd, Channel* channel> **Channels_**     

<font color=blue>**EPollPoller**</font>(EventLoop *loop) 

​	vector<epoll_event>  events_	// epoll_event是一个结构体，存发生事件events     
​	int epollfd_     

> 使用哈希表能使得监听查找的更快
> Poller是基类；EPollPoller是派生类，包含Poller的具体实现

***

<font color=blue>**Thread **</font>(functional<void()> &cb, string &name) 

​	bool **started_**     
​	bool **joined_**     
​	shared_ptr< std::thread> **thread_**     
​	pid_t **tid_**     
​	fucntion<void()> **func_**     
​	string **name_**     
​	atomic_int **numCreated_**     

<font color=blue>**EventLoopThread**</font>(functional<void(EventLoop*)> &cb, string &name)

​	EventLoop* **loop_**     
​	Thread **thread_**     
​	ThreadInitCallback **callback_**     

***

<font color=blue>**EventLoopThreadPool**</font>(EventLoop* baseLoop,   string &nameArg)

​	EventLoop* **baseLoop_**     
​	string **name_**     
​	int **numThreads_**     
​	int **next_**     
​	vector<unique_ptr<EventLoopThread>> **threads_**     
​	vector<EventLoop*> **loops_**     

> 一个thread对应一个loop

***

<font color=blue>**Socket**</font>(int sockfd)

​	int **sockfd_**     

***

<font color=blue>**Acceptor**</font>(EvnetLoop *loop,    InetAddress &listenAddr,    bool reuseport)

​	EventLoop ***loop_**  // Acceptor使用的就是用户定义的base Loop，也称为main Loop     
​	Socket **acceptSocket_**     
​	Channel **acceptChannel_**     
​	NewConnectionCallback **NewConnectionCallback_**     
​	bool **listenning_**     

> 主要地，创建了socket，以及封装到了channel中，绑定了监听的地址，设置了链接回调函数
> 其只关注新链接，只关注读事件

***

<font color=blue>**Buffer**</font>(size_t initialSize = 1024)

​	size_t **kCheapPrepend** = 8     
​	size_t **readerIndex_**     
​	size_t **writerIndex_**     

> 应用写数据→缓冲区→Tcp发送缓冲区→send

***

<font color=blue>**TcpConnection**</font>(EventLoop *loop,      string &name,     int sockfd, 
				InetAddress& localAddr_ ,   InetAddress& peerAddr_)

​	EventLoop* **loop_**     
​	string **name_**     
​	stomic_int **state_**     
​	unique_ptr<Socket> **socket_**     
​	unique_ptr<Channel> **channel_**     
​	InetAddress **localAddr_**     
​	InetAddress **peerAddr_**     
​	xxxCallback **xxxCallback_**     
​	size_t **highWaterMark_**     
​	Buffer **inputBuffer_**     
​	Buffer **outputBuffer_**     

> 一个链接成功的客户端对应一个TcpConnection

***

<font color=blue>**TcpServer**</font>(EventLoop* loop,   InetAddress &listenAddr,   string nameArg,   Option option)

​	EventLoop* **loop_**  // 也就是baseloop     
​	string **ipPort_**     
​	string **name_**     
​	unique_ptr<Acceptor> **acceptor_**   // 运行在mainLoop中，监听新连接事件     
​	shared_ptr<EventLoopThreadPool> **threadPool_**  // one loop per thread     
​	unordered_map<string, TcpConnectionPtr> **connections_**     

> 总领所有，应用实现时主要从这里开始修改

***

## 函数实现
总结来说，TcpServer中大部分的具体实现在TcpConnection中，Channel相关的具体操作实现在Channel中，上一层的类更多是封装和调用。


# 服务器性能测试 🔍
可以使用QPS(query per second)进行衡量，以下两个就是服务器的压力测试工具。在测试的过程中，会涉及进程socketfd相关的设置，因为Linux中一个进程的fd数量设置具有上限。如果想进一步提升性能，提高并发量，就要考虑分布式或者集群部署。

- wrk		Linux上，需要单独编译安装；但只能测试http服务的性能
- Jmeter           需要先安装JDK，再安装Jmeter；可以测试http和tcp服务，可以生成聚合报告

> 在测试时，需要使用两台机器进行测试，因为测试工具也要占用大量资源
