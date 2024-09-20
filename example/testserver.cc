#include <mymuduo/TcpServer.h>
#include <mymuduo/Logger.h>

#include <functional>

class EchoServer
{
public:
    EchoServer(EventLoop* loop,
            const InetAddress &addr,
            const std::string &name)
        : server_(loop, addr, name)
        , loop_(loop)
    {
        // 注册回调函数
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1)
        );

        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3)
        );

        server_.setThreadNum(3);

        // 设置合适的loop线程数量
    }

    void start()
    {
        server_.start();
    }
private:

    // 连接建立 或 断开 的回调
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO("conn UP: %s", conn->peerAddr().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN: %s", conn->peerAddr().toIpPort().c_str());
        }
    }

    // 可读写事件的回调
    void onMessage(const TcpConnectionPtr &conn,
                Buffer* buf,
                Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        LOG_INFO("onMessage. buf->msg: %s \n", msg.c_str());
        conn->send(msg);
        conn->shutdown(); // 关闭写端 -> EPOLLHUP
    }

    // 

    EventLoop* loop_;
    TcpServer server_;
};

int main(void)
{
    EventLoop loop;
    InetAddress addr(8000);
    
    // 主要是创建 Acceptor实例
    EchoServer server(&loop, addr, "EchoServer-01");
    
    // 主要工作：1. 创建子线程，subloop，同时开启loop.loop() 2. Acceptor listening
    server.start();
    loop.loop(); // mainloop 开启事件循环

    return 0;
}