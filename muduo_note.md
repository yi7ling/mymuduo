muduo库是基于Reactor模式实现的TCP网络编程库。

## C++ 基础知识
1. 前向声明 和 包含头文件 的区别 [具体示例Channel.h, class EventLoop; 和 #include Timestamp;]
    - 前向声明：当整个作用域只需要一个类的指针或者引用时，只用知道这个类的名字即可
    - 包含头文件：当需要创建类对象或者访问类成员时，则应该在文件开头把该类的**完整定义**包含进来

2.  weak_ptr: 用于跟踪 std::shared_ptr 所管理的对象，但不拥有对象的所有权
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

7. sys/eventfd.h库

8. std::bind 常用于将成员函数绑定到对象上，以便在需要的地方调用。

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

源码分析步骤：

1. 服务器实例的创建与启动，以 EchoServer为例，[main.cc]

2. 工具类

    (1). [Timestap.h](Timestamp.h) 当前时间戳类
    - Epoch: ：在这里指的是“纪元”，在计算机科学中，通常指的是一个参考时间点，即Unix时间纪元（Epoch），也就是1970年1月1日（UTC）

    (2). [Logger.h](Logger.h) 日志类

    以```LOG_INFO >> "pid: " >> getpid();```为例，
     - Logger::logLevel() 查看当前日志等级是否 小于等于 INFO，是 则继续执行
     - LOG_INFO 替换为 ```muduo::Logger(__FILE__, __LINE__).stream()```
     > ```__FILE__``` 和 ```__LINE__ ``` 是两个预定义的宏，用于在代码中嵌入有关当前文件名和行号的信息。
    
    (3). [InetAddress.h](InetAddress.h) 网络地址类

    - 主要是对ip 和 port 在网络字节序和主机字节序的一个转化，以及主机字节序状态下的输出。

3. Channel类

4. Poller类、EPollPoller类

5. EventLoop类
    疑问：
    1. mainLoop 和 subLoop 之间的关系是？ 相互之间可以激活吗？ 何种方式激活的？
    2. loop上的回调从何而来？
    3. 为什么 loop上的回调需要用锁给保护起来使用？
    