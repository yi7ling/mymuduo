#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <string.h>
#include <sys/socket.h>

EventLoop* checkMainLoopNotNull(EventLoop* loop)
{
    if (loop == NULL)
    {
        LOG_FAIAL("mainloop is null!\n");
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const std::string& nameArg,
            Option option)
            : loop_(checkMainLoopNotNull(loop))
            , ipPort_(listenAddr.toIpPort())
            , name_(nameArg)
            , acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
            , threadPool_(new EventLoopThreadPool(loop, name_))
            , connectionCallback_()
            , messageCallback_()
            , nextConnId_(1)
            , started_(0)
{
    /**
     * 在acceptor中设置新连接的回调 （Acceptor::handleRead() -> newConntectionCallback_）
     * placeholders 用于设置参数占位符
     */
    acceptor_->setNewConntectionCallback(std::bind(&TcpServer::newConnection, this,
        std::placeholders::_1, std::placeholders::_2));
}

/**
 * 将TcpServer保存的TcpConnection连接都释放掉
 */
TcpServer::~TcpServer()
{
    LOG_INFO("TcpServer::~TcpServer [\"%s\"] destructing \n",
            name_.c_str());

    for (auto& item: connections_)
    {
        TcpConnectionPtr conn(item.second);

         // 把原始的智能指针复位，
         // 让栈空间的TcpConnectionPtr conn指向该对象 
         // 当conn出了其作用域 即可释放智能指针指向的对象。
         // 目的是释放保管的TcpConnection对象的共享智能指针
        item.second.reset();

        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn)
        );
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if (started_++ == 0) // 防止一个TcpServer对象被 start多次
    {
        threadPool_->start(threadInitCallback_); // 启动底层的loop线程池

        // 启动mainloop中 acceptor监听连接
        loop_->runInLoop(
            std::bind(&Acceptor::listen, acceptor_.get()));

        // acceptor_->listen(); // 和上面的区别是什么
    }
}

// acceptor执行 handleRead 中的回调 newConntectionCallback_
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    EventLoop* ioLoop = threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_; // 只有mainloop所在线程处理，所以不需要做原子操作

    std::string connName = name_ + buf;


    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] form %s \n",
            name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    
    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    memset(&local, 0, sizeof local);
    socklen_t addrlen = sizeof local;

    if (::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }

    InetAddress localAddr(local);

    // 新建连接
    TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));
    connections_[connName] = conn;

    // 设置用户提供的回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // 设置关闭连接的回调
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)
    );
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop,
                    this, conn)
    );
}

// mainloop所在线程执行
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",
        name_.c_str(), conn->name().c_str());

    size_t n = connections_.erase(conn->name());
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn)
    );
}
