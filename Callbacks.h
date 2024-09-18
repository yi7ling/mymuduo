#pragma once

/**
 * 定义回调的类型
 */

#include <memory>
#include <functional>

#include "Timestamp.h"

class Buffer;
class TcpConnection;

// 所有客户端可见的callback

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void (const TcpConnectionPtr&)>;
using CloseCallback = std::function<void (const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void (const TcpConnectionPtr&)>;

// the data has been read to (buf, len)
using MessageCallbck = 
    std::function<void (const TcpConnectionPtr&,
                        Buffer*,
                        Timestamp)>;

using HighWaterMarkCallback =
    std::function<void (const TcpConnectionPtr&,
                        size_t)>;
