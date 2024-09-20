muduo库是基于Reactor模式实现的TCP网络编程库。

## C++ 基础知识
1. 前向声明 和 包含头文件 的区别 [具体示例Channel.h, class EventLoop; 和 #include Timestamp;]
    - 前向声明：当整个作用域只需要一个类的指针或者引用时，只用知道这个类的名字即可
    - 包含头文件：当需要创建类对象或者访问类成员时，则应该在文件开头把该类的**完整定义**包含进来

2. 智能指针
    C++的智能指针本质是对原始指针的一种封装。
    C++智能指针主要有两种：unique_ptr, shared_ptr（c++14支持，c++11不支持）

    unique_ptr：1.作用域指针，超出作用域就会被delete
    
                2. 唯一的，不可复制的

    shared_ptr: 工作方式通过引用计数

    weak_ptr: 1. 可以和共享指针一起使用

              2. weak_ptr的引用，不会使 引用计数器＋1

              3. 主要用于跟踪 std::shared_ptr 所管理的对象，但不拥有对象的所有权
    ```
    std::weak_ptr<void> tie_;
    bool tied_;
    ```

3. 左右值, 移动语义

4. const类型的成员函数中，用到了map中的 hashmap[key]会报错吗？

    解释：如果你在 const 成员函数中使用 hashmap[key]，编译器会报错，因为它违反了 const 成员函数的承诺，因为它可能会插入一个新的键值对到 map 中，如果 fd 这个键在 map 中不存在的话。如果你想在 const 成员函数中访问 map，你应该使用 at() 方法或者 find() 方法，这两种方法都不会修改 map

5. ```extern __thread int t_cachedTid;```

    extern: 引用外部定义，这里只做声明。
    
    __thread: 这是一个指示编译器该变量**具有线程局部存储的关键字**。这意味着每个线程都将拥有这个变量的独立副本。

6. atomic库
    CAS是“Compare-And-Swap”的缩写，是一种常用于**并发编程中的原子操作**。CAS操作包含三个参数：内存位置（V）、预期原值（A）和新值（B）。

7. eventfd()的使用

    ```cpp
    /* eventfd - create a file descriptor for event notification */
    #include <sys/eventfd.h>
    int eventfd(unsigned int initval, int flags);

    ```

    调用函数eventfd()会创建一个eventfd对象，或者也可以理解打开一个eventfd类型的文件，类似普通文件的open操作。eventfd的在内核空间维护一个无符号64位整型计数器， 初始化为initval的值。

8. std::bind 常用于将成员函数绑定到对象上，以便在需要的地方调用。
    ```cpp
    struct Foo {
        void print_sum(int n1, int n2)
        {
            std::cout << n1+n2 << '\n';
        }
        int data = 10;
    };

    int main() 
    {
        Foo foo;
        auto f = std::bind(&Foo::print_sum, &foo, 95, std::placeholders::_1);
        f(5); // =100
    }
    ```

    总结：Foo::print_sum是成员函数本身，而&Foo::printf_sum是指向该成员函数的指针，这需要将成员函数作为函数模板（如std::bind）时是必要的。

9. semaphore、condition_variable以及mutex库
    semaphore: 信号量机制，通过设置信号量数实现线程间的并发访问数量
    condition_variable: 条件变量，主要用于实现线程间的同步。如：生产者消费者问题

    条件变量使用过程：
    1. 拥有条件变量的线程获取互斥量；
    2. 循环检查某个条件，如果条件不满足则阻塞直到条件满足；如果条件满足则向下执行；
    3. 某个线程满足条件执行完之后调用notify_one或notify_all唤醒一个或者所有等待线程。
       条件变量提供了两类操作：wait和notify。这两类操作构成了多线程同步的基础。

    mutex: 互斥锁，主要用于实现线程间互斥访问共享资源

    mutex的使用：mutex.lock() 和 mutex.unlock()，但相对于手动lock和unlock，
    我们可以使用RAII(通过类的构造析构， Resource Acquisition Is Initialization)来实现更好的编码方式，可以使用unique_lock,lock_guard 实现自动的加锁和解锁。

## muduo库前置知识

Reactor模式：
    Reactor 是一种高效的事件处理模式。释义 “反应堆”，是一种事件驱动机制

事件驱动：
    简而言之是你按了什么按钮（即产生了事件），电脑执行了什么操作（即调用了什么函数）

> [两种高效的事件处理模式(reactor模式、proactor模式)](https://blog.csdn.net/qq_41453285/article/details/103001772?ops_request_misc=&request_id=&biz_id=102&utm_term=reactor%E6%A8%A1%E5%BC%8F&utm_medium=distribute.pc_search_result.none-task-blog-2~all~sobaiduweb~default-4-103001772.142^v100^pc_search_result_base3&spm=1018.2226.3001.4187)

网络编程中常用的fd是什么：
    File Discriptor(文件描述符)

IO多路复用(IO Multiplexing)：
    多路是指网络连接，复用是指线程/进程。多个网络连接用同一个线程/进程进行处理，从微观上来说，虽然一个线程在一个时刻只能处理一个网络请求，但处理时间非常短，在1s 内能够处理上百上千个IO，从宏观上看，多个IO请求复用了一个线程/进程，这就是多路复用。这种思想类似一个cpu并发多个进程，即时分多路复用。

> 详细的IO多路复用解释见 [IO多路复用](https://blog.csdn.net/m0_51319483/article/details/124264619?ops_request_misc=%257B%2522request%255Fid%2522%253A%252208951B96-1D9A-479F-9284-91955CD0CD44%2522%252C%2522scm%2522%253A%252220140713.130102334..%2522%257D&request_id=08951B96-1D9A-479F-9284-91955CD0CD44&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~sobaiduend~default-3-124264619-null-null.142^v100^pc_search_result_base3&utm_term=%E4%BB%80%E4%B9%88%E6%98%AFi%2Fo%E5%A4%9A%E8%B7%AF%E5%A4%8D%E7%94%A8&spm=1018.2226.3001.4187)


epoll库：
    epoll, 即Linux epoll I/O事件通知机制的组件之一，常用于事件驱动编程。
    
- ```struct epoll_event;```   用于epoll机制的关键结构体，通常与 ```epoll_ctl()``` 和 ```epoll_wait()``` 函数一起使用
- ```epoll_create()``` 创建一个epoll实例
- ```epoll_ctl()```   用于管理fs事件
- ```epoll_wait()```  用于监听fs事件

socket和addr（IP地址:端口号）之间的关系：
    1. socket表示通信端点，每个socket都有一个文件描述符，操作系统通过这个文件描述符来管理网络通信
    2. addr包含了网络通信所需的地址信息，包含IP地址和端口号
    3. 在服务器端，bind() 函数用于将 socket 与一个本地地址（addr）绑定。
    这告诉操作系统，所有发送到这个地址的网络数据都应该由这个 socket 来处理。

同步调用和异步调用：
```cpp
// 在mainloop所在的线程中执行listen。异步调用
loop_->runInLoop(
    std::bind(&Acceptor::listen, acceptor_.get()));

// 直接在当前线程中执行listen, 同步调用
acceptor_->listen();
```

muduo底层中，可读可写事件的触发是由谁引起的？

muduo底层是由epoll驱动的，可读可写的状态是由fd对应的内核缓冲区决定的。
触发方式：首先要调用 epoll_ctl 注册需要关注的事件类型，

1. 当关注了可读事件，fd对应的缓冲区从空变成非空，则触发了读事件
2. 当关注了可写事件，fd对应的缓冲区从满变成非满，则触发了写事件

epoll的触发模式有哪些：

epoll的触发方式有两种：水平触发LT、边缘触发ET

- LT: epoll的默认模式，在LT模式下，只要fd 的状态与事件类型匹配，epoll_wait() 调用就会不断返回该事件。

    例如：如果一个 socket 上有数据可读，那么每次 epoll_wait() 被调用时，只要缓冲区中还有未读取的数据，它就会返回读事件（EPOLLIN）。

- ET: epoll_wait() 仅在fd 的状态发送变化时返回事件。

    例如：对于写事件，只要 socket 的发送缓冲区从不可写状态变为可写状态，epoll_wait() 就会返回一次写事件（EPOLLOUT），之后除非缓冲区再次变为不可写状态，否则不会再次返回写事件。


## 源码分析

### 1. 服务器实例的创建与启动
    
以 EchoServer为例，[testserver.cc](example/testserver.cc)

① 先通过运行 autobuild.sh 构建muduo库的头文件和动态库

② 在 example文件夹下，执行 make 命令，构建起一个muduo服务

③ 新建一个shell窗口，执行 ```telnet 127.0.0.1 服务器提供的端口```，查看运行的日志

### 2. 工具类

(1). [Timestap.h](Timestamp.h) 当前时间戳类
- Epoch: ：在这里指的是“纪元”，在计算机科学中，通常指的是一个参考时间点，即Unix时间纪元（Epoch），也就是1970年1月1日（UTC）

(2). [Logger.h](Logger.h) 日志类

主要关注：

1. 设置日志级别
2. 单例模式创建Logger
3. 宏定义的灵活使用

(3). [InetAddress.h](InetAddress.h) 网络地址类

- 主要是对ip 和 port 在网络字节序和主机字节序的一个转化，以及主机字节序状态下的输出。

### 3. Channel类

关键成员变量：fd、events、revents、读/写/关闭/错误 的callback



### 4. Poller类、EPollPoller类 - Demultiplex

关键成员变量：std::unoredered_map<int, Channel*> channels

### 5. EventLoop类 - Reactor
    疑问：
    1. mainLoop 和 subLoop 之间的关系是？ 相互之间可以激活吗？ 何种方式激活的？
    2. loop上的回调从何而来？
    3. 为什么 loop上的回调需要用锁给保护起来使用？
    4. 为什么EventLoop类中的 looping_、quit_字段 要设置成原子类型

关键成员变量：activeChannels、wakeupfd_、wakeupChannel

### 6. Thread 和 EventLoopThread

- Thread主要封装了c++ 的 thread方法
- EventLoopThread主要对，Thread 和 EventLoop进行了封装，其中关键的函数 threadFun() 实现了 one loop per thread

### 7. EventLoopThreadPool

关键成员变量：eventloop, thread, numThreads

getNext(): 以轮询方式获取一个 ioloop

### 8. socket

关键成员变量：sockfd

主要是封装 socket fd, socket.h 是Linux网络编程重要的库，提供了一些使用套接字的方法。

### 10. Acceptor

关键成员变量：acceptSocket_, acceptChannel

> 注意区别 Acceptor中的 handleRead() 和 TcpConnection 中的 handleRead()

主要是封装listenfd的相关操作，socket、bind、listen、baseLoop

### 11. Buffer

缓冲区 vector<char> buf
    prependable  readeridx  writeridx


服务器读数据：

    客户端写数据 -> connfd缓冲区 -> inputBuffer -> 服务端接收

服务器写数据：

    服务端发送数据 -> send() -> outputBuffer（如果一次性无法写到connfd缓冲区） -> connfd缓冲区 -> 客户端接收数据

### 12. TcpConnection

关键成员变量：sockfd, channel, handle读/写/关闭/错误 的回调 --> 对应Channel中的callback_变量, 读写缓冲区

### 13. TcpServer

关键成员变量：Acceptor, ConnectionMap connections_, threadPool_

    