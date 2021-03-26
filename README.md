# metro

此项目是本人用于学习Linux网络编程，而仿照着`trantor`库复写的项目，源代码几乎一致。因为本意是学习Linux，所以删减了跨平台代码支持，由于对SSL还未学习，现阶段SSL也未加入到源码当中。其他人可以通过本项目了解到`trantor`库的设计思路和实现细节，来帮助各位更有效地阅读`trantor`库源代码。

## metro项目结构

主要源码文件如下：

```cpp
.
├── CMakeLists.txt									
├── README.md										
└── metro
    ├── net
    │   ├── Channel.cc									
    │   ├── Channel.h									// IO 调度控制块
    │   ├── EventLoop.cc								
    │   ├── EventLoop.h									// EventLoop用于调度Channel
    │   ├── EventLoopThread.cc		
    │   ├── EventLoopThread.h							// one thread one loop，用于在线程启动EventLoop
    │   ├── EventLoopThreadPool.cc
    │   ├── EventLoopThreadPool.h						// ThreadPool，用于管理所有EventLoopThread
    │   ├── InetAddress.cc
    │   ├── InetAddress.h								// 对IP地址结构的封装，支持IPv4和Ipv6
    │   ├── TcpClient.cc
    │   ├── TcpClient.h									// 封装了Connctor、TcpConnection， 提供便利的Client模型接口
    │   ├── TcpConnection.cc							
    │   ├── TcpConnection.h								// 封装了EventLoop，Channel,用于管理一个以建立连接socket
    │   ├── TcpServer.cc
    │   ├── TcpServer.h									// 封装了Acceptor、TcpConnection，提供便利的Server模型接口
    │   ├── callbacks.h									// 所有的callback类型定义
    │   └── inner
    │       ├── Acceptor.cc								
    │       ├── Acceptor.h								// 提供监听socket的callback模型
    │       ├── Connector.cc
    │       ├── Connector.h								// 提供socket的connect事件回调模型
    │       ├── Poller
    │       │   ├── EpollPoller.cc						// 封装了epoll，定义了Poller的子类
    │       │   └── EpollPoller.h
    │       ├── Poller.cc								
    │       ├── Poller.h								// 定义了Poller的统一接口，主要目的是跨平台，这里体现不到
    │       ├── Socket.cc
    │       ├── Socket.h								// 封装了socket接口
    │       ├── TcpConnectionImpl.cc
    │       ├── TcpConnectionImpl.h						// TcpConnection的具体实现类
    │       ├── Timer.cc
    │       ├── Timer.h									// 协助完成定时器功能
    │       ├── TimerQueue.cc
    │       └── TimerQueue.h							// 用于保存Timer，在EventLoop中使用到，实现了EventLoop的定时器功能
    └── utils
        ├── Date.cc										
        ├── Date.h										// 用于获取系统时间
        ├── Funcs.h	
        ├── LogStream.cc								
        ├── LogStream.h									
        ├── Logger.cc
        ├── Logger.h									// Logger、LogSteam共同实现了一个简易的Log系统
        ├── MpscQueue.h									// 实现了一个异步安全队列
        ├── MsgBuffer.cc
        ├── MsgBuffer.h									// 消息缓冲队列，用于保存socket中接收到的数据
        ├── NonCopyable.h
        ├── TimingWheel.cc								
        └── TimingWheel.h								// 看门狗结构体，用于释放长时间未活动的socket
```

此处删除了test目录，此目录是编写代码过程中做的各个功能测试

整个项目可以分为两个部分，一个部分为`EventLoop`部分，其实现了epoll的封装，构建了基本的IO回调架构。

另一部分为TcpClient、TcpService部分，这部分在EventLoop的基础上进一步封装，针对socket模型提供了更为便利的调用接口

### EventLoop部分

eventloop部分涉及到源代码Channel.cc、EventLoop.cc、Timer.cc、TimerQueue.cc、Poller.cc、EpollPoller.cc等关键文件。

**Channel.cc**

Channel.cc中主要包含三个关键成员变量，`events_`，`fd_`，`revents_`和一系列的callback。`fd_`中存储Channel所负责的文件描述符，`events_`表示Channel在epoll中注册的监听事件，其可以通过`update()`结构将其值更新到epoll当中。`revents_`保存每次`epoll_wait`操作之后返回的事件类型，在`handleEvent()`中会根据返回状态调用相应的callback。

**EpollPoller.cc和Poller.cc**

`Poller`和`EpollPoller`两个对象是基类与派生类的关系，Poller提供统一的外部调用接口，我们在外部调用时也是调用的Poller，其能够保证如果平台没有Epoll的情况下，我们只要实现了Poller的接口，便可实现代码的平台迁移。EpollPoller.cc则是对Epoll的封装，查看这个文件可以看到我们是如何封装epoll的。

**EventLoop**

这个是中央调度器，它提供了我们所需的所有IO调度接口。其主要有如下几个功能点组成

*updateChannel()*

 这个接口是EventLoop提供，但是被封装在了Channel中，在外部调用时我们一般时直接针对Channel调用来删除、添加监听事件。其实现原理就是调用其内部存储的` poller_`的`updateChannel`方法，这里我们需要意识到one thread one poll，updateChannel接口并不是线程安全地，其仅能在loop中调用，来避免data race。异步更新更新时需要使用`runInLoop`

*runInLoop*

EventLoop对其接口进行了明显的行为限定，大多数接口只能在loop thread中才能访问，这些接口在使用时都添加了`assertInLoopThread()`。如果其他线程想要操作EventLoop，可以通过`func_`来进行操作，其是一个`void()`类型的函数对象Queue，支持异步插入，我们可以向其插入我们想要执行的函数，EventLoop会在其线程循环中自动执行插入函数，由于这一执行过程均是在EventLoop Thread中完成，所以不存在数据竞争情况。具体实现中，利用eventfd和mutex queue（原作者用的是lockfree queue，这个我不会，替换成了mutex queue）。epoll中会保存监听eventfd，这样便可以保证EventLoop对runInLoop的即使操作。

*runEvery， runAfter*

EventLoop中还实现了定时器功能，其能够在固定事件之后、或者每隔多少事件之后自动执行某些函数。要实现这一功能需要使用Timer，其用于存储延迟函数、调用时间点等信息，TimerQueue：其是一个Timer的基于时间点优先队列和timefd这一功能是基于Linux系统接口的timerfd完成，epoll中监听了此timefd，其readcallback时间为TimerQueue最早过期的时间，当readcallback调用时对其内部重排，移除所有的过期Timer（这里移除的可能不止一个，因为存在调用延迟），然后设置下一个Timer的过期时间为timefd的readcallback时间点。通过这种方式我们只需要一个timerfd便可以完成所有的延迟监听。

*loop*

这个函数便是EventLoop运行的关键函数，其每次调用poller->poll，对于那些返回的Channel调用其handleEvnet()来处理相应的调用事件。除此之外其还会处理`runInLoop`中插入的callback事件。

**EventLoopThread  EventLoopThreadPool**

evnetloop创建接口，用于启动线程运行eventloop，EventLoopThreadPool是evenloop的线程池，我们每次想要获取eventloop时可以通过其`getNextLoop()`来循环换取EventLoop，避免使得某一线程中的eventloop负载过重。

## TcpClient、TcpService

这一部分就是利用EventLoop机制实现，封装socket调用接口，其封装出的对象如下

**Connector**

实现socket的连接操作，对于非阻塞套接字的connect连接操作，其一般流程为`set unblock`->`connect`->`write`->`如果getsockopt返会0表示连接成功`。因此Connector的实现原理最关键部分便是`writecallback`的处理，正常情况下当连接建立成功时，epoll会自动调用其Connector中的Channel.WriteCallback。此函数还会连锁调用`newConnectionCallback_`。

**Acceptor**

非阻塞socket的listen，放置在epoll中，当有连接到来时其会设置为readable，所以只需设置其相应Channel的ReadCallback便可以实现连接的自动处理，当有新连接建立完成时`newConnectionCallback`也会调用。

**TcpConnectionImpl **

当连接建立完成时，我们便可以利用socket进行数据通信，此时通信接口全部由此类提供。通信接口主要由两大类组成，write和read。

*write*

其内部提供了许多send函数，这些函数便是用于实现数据发送的。我们知道非阻塞socket的write操作可能发生数据的部分发送，为了在send层屏蔽掉这些，我们需要为send函数提供缓冲机制，即将未发送完的数据缓存在TcpConnectionImpl当中，这里实现利用了一个`BufferNode`链表，未发送完的数据会存储在node的buffer当中。当有数据进入缓冲链表是，WriteCallback便会设置，其用于不断地将缓冲区数据向socket写入。这里需要注意的一点是数据的顺序一致性，当有数据已经缓冲在链表当中时，我们不能再进行直接发送，而是应放入缓冲队列的末尾。

write还可以进行文件的传输，其利用了sendfile接口，零拷贝技术能够让其更加快速并且避免了CPU时间的浪费。为了实现这个，BufferNode其实是一个复用结构，其既能表示未发送完全的数据是buffer（BufferNode.sendFd_ == -1）、也可以表示其是一个未发送完全的文件(BufferNode.sendFd_ == -1)

*read*

read设计相对简单，其就是向相应channel注册了ReadCallback，当其调用时表示有数据可读，此时我们需要将数据从内核拷贝到我们自己的缓冲区当中。``

**TcpClient**

将Connector和TcpConnectionImpl封装为了一个整体，Connector的`newConnectionCallback_`中连接了TcpConnectionImpl的初始化代码，这样便实现了连接成功之后的自动初始化行为。接口中的setCallback函数都是设置的TcpConnectionImpl的callback。

**TcpServer**

不同于TcpClient的是，其内部包含了一个`EventLoopThreadPool`，Acceptor生成`TcpConnectionImpl`时，其会从Pool中去Eventloop，这样能够保证处理的链接套接字都能够均匀分布在各个thread的pool当中。