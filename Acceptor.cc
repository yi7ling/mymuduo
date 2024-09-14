#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FAIAL("socket create err:%d", errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool request)
    : loop_(loop)
    , acceptSocket_(createNonblocking()) // 创建socket
    , acceptChannel_(loop, acceptSocket_.fd())
    , listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(request);
    acceptSocket_.bindAddress(listenAddr); // 绑定套接字

    // TcpServer::start() -> Acceptor::listen()
    // 有新用户连接，需要执行回调Acceptor::handleRead 
    // (回调具体做什么事情是由TcpServer提供的，比如connfd -> channel -> subloop)
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen()
{
    listenning_ = true;
    acceptSocket_.listen(); // 监听连接
    acceptChannel_.enableReading(); // acceptChannel_ => Poller
}

// listenfd有事件发生了，就是有新用户连接了
void Acceptor::handleRead()
{
    InetAddress peerAddr;

    // TODO 疑问？ peerAddr 和 上面listenAddr有什么关系吗？
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        if (newConntectionCallback_)
        {
            // 执行TcpServer提供的回调
            // 轮询找到subLoop，唤醒，分发当前的新客户端的channel
            newConntectionCallback_(connfd, peerAddr);
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        if (errno == EMFILE)
        {
            LOG_ERROR("sockfd reached limit. \n");
        }
    }
}
