#include "TcpServer.h"
#include "Logger.h"

#define CHECK_NOTNULL(x) \

EventLoop* checkLoopNotNull(EventLoop* loop)
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
            : loop_(checkLoopNotNull(loop))
            , ipPort_(listenAddr.toIpPort())
            , name_(nameArg)
            , acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
            , threadPool_(new EventLoopThreadPool(loop, name_))
            , connectionCallback_()
            , messageCallback_()
            , nextConnId_(1)
{
    /**
     * 在acceptor中设置新连接的回调 （Acceptor::handleRead() -> newConntectionCallback_）
     * placeholders 用于设置参数占位符
     */
    acceptor_->setNewConntectionCallback(std::bind(&TcpServer::newConnection, this,
        std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{

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