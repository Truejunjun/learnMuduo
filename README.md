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

<font color=blue>**Poller**</font>(EventLoop *loop)  →  事件分发器Demultiplex

​	EventLoop* **ownerloop_**     
​	unordered_map<int fd, Channel* channel> **Channels_**     

<font color=blue>**EPollPoller**</font>(EventLoop *loop) 

​	vector<epoll_event>  events_	// epoll_event是一个结构体，存发生事件events     
​	int epollfd_     

> 使用哈希表能使得监听查找的更快
> Poller是基类；EPollPoller是派生类，包含Poller的具体实现

***

<font color=blue>**Thread**</font>(functional<void()> &cb, string &name) 

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

**启动并注册Acceptor监听新事件：**`TcpServer`会创建一个`mainLoop`，然后创建`Acceptor`去监听新的事件，并且`Acceptor`的新连接回调函数就是由`TcpServer`设置。随后，`Acceptor`会创建一个`socket`并用其中的`fd`封装成`channel`，然后送给`Poller`监听，并且还使能`Channel`的读操作，通过注册读事件将listenfd注册进Poller。

> 实际上，`Acceptor`在构造时设置了回调函数为自身的`Acceptor::handleRead()`，而`handleRead()`中关键执行了`newConnectionCallback`，而这个回调函数实际上是由`TcpServer`设置的自身的`TcpServer::newConnection`函数。而这个函数实际上是完成了`TcpConnection`的创建和设置，最终执行`TcpConnection::connectEstablished`
>
> ![img](https://github.com/Truejunjun/learnMuduo/blob/main/pictures/%E6%96%B0%E7%94%A8%E6%88%B7%E9%93%BE%E6%8E%A5%E6%A2%B3%E7%90%86.png)

**监听后建立新连接：**Poller监听到链接客户端的`connfd`，执行相应回调`newConnectionCallback_(connfd, peerAddr)`，该函数则首先通过轮询方法指定一个`subLoop（ioLoop）`，然后通过`connfd`获取链接的ip和端口号，创建新的`TcpConnection`类，封装`connfd`，最后完成回调函数的设置。最后走到`TcpConnection`中的`connectEstablished()`函数中。

> **connfd封装：**封装主要是`TcpConnection`类，主要封装了socket、sockfd、Addr和回调函数，其中`ConnectionCallback`回调操作一送入子线程就会执行，而`MessageCallback`则是有新数据才会执行。
>
> 对于**回调设置操作**，**与`Acceptor`相似**，`Acceptor`工作于`mainLoop`中，`TcpConnection`工作于`subLoop`中，其中都需要封装`socket`。其中有很多回调操作与`TcpServer`一样，这是因为`TcpServer`→ `Acceptor` → 有一个新用户链接，通过`accept`函数拿到`connfd`，随后封装得到`TcpConnection`，设置相应回调 → `Channel` →` Poller` → `Channel`回调操作。**简单来说**，就是`TcpServer`设置的回调函数送给`TcpConnection`。

**唤醒子线程并分发事件：**上一步唤醒了`newConnection`操作，如果设置了多线程，那么则用`ioLoop = threadPool_->getNextLoop()`指针方法选择线程，若指向`mainLoop`则可以直接运行`runInLoop`，若指向别的线程，则通过`queueInLoop`执行事件。但由于每一个线程都是阻塞于`epoll_wait`的监听上，所以需要通过给`wakeupfd_`事件写入8字节数据唤醒线程，将connfd事件封装好再分发给子线程。

**对于关闭操作**，用户执行了关闭`shutdown`后，会执行`TcpConnection`中的`shutdown()`，设置状态为断开链接中，然后执行`shutdownInLoop()`，关闭写操作，这样就会产生`EPOLLHUP`事件给`Poller`监听到，处理相应的关闭回调函数`channel::closeCallback_`，也就是`TcpConnection`中的`handleClose()`，也就是当初在`TcpServer`中设置好的回调函数`removeConnection`，然后通过完成具体操作的`removeConnectionInLoop`调用`TcpConnection`中的`connectionDestroyed()`禁止`channel`的事件并移除`channel`.

> TcpServer就完成了connection的删除,具体channel的操作是由TcpConnection完成的

![img](https://github.com/Truejunjun/learnMuduo/blob/main/pictures/%E6%A8%A1%E5%9D%97%E4%BA%A4%E4%BA%92%E6%A2%B3%E7%90%86.png)


其中关键的函数为`start()`，先启动了底层的线程池，注册了`wakeupFd`，创建并唤醒`Loop`子线程并开启`loop.loop()`。然后就执行了`acceptor.listen()`，将`acceptChannel`注册在`mainLoop`上，最终开启`mainLoop`的`loop()`。**简单来说就是**，构建`TcpServer`→`start`→运行`mainLoop`

# 服务器性能测试 🔍
可以使用QPS(query per second)进行衡量，以下两个就是服务器的压力测试工具。在测试的过程中，会涉及进程socketfd相关的设置，因为Linux中一个进程的fd数量设置具有上限。如果想进一步提升性能，提高并发量，就要考虑分布式或者集群部署。

- wrk		Linux上，需要单独编译安装；但只能测试http服务的性能
- Jmeter           需要先安装JDK，再安装Jmeter；可以测试http和tcp服务，可以生成聚合报告

> 在测试时，需要使用两台机器进行测试，因为测试工具也要占用大量资源
