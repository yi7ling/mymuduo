#include "Logger.h"
#include <iostream>

#include "Timestamp.h"

// 获取日志的唯一实例
Logger& Logger::instance()
{
    // 局部静态变量，注意它的生命周期和作用域
    static Logger logger;
    return logger;
}

// 设置日志级别
int Logger::setLogLevel(int level)
{
    loglevel_ = level;
}

// 写日志   [日志级别]  time : msg
void Logger::log(std::string msg)
{
    switch (loglevel_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FAIAL:
        std::cout << "[FAIAL]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
    default:
        break;
    }

    std::cout << Timestamp::now().toString() << msg << std::endl;
}

