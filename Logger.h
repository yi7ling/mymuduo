#pragma once

// 区别C语言标准库的头文件 <string.h>
#include <string>

#include "noncopyable.h"



/**
 * 通过宏定义方式简化日志工具的使用
 * ##__VA_VRGS__是一种宏定义中处理可变参数列表的方式，比如 
 * snprintf(buf, 1024, "Error %d: %s", 404, "Not Found")
 */
#define LOG_INFO(logmsgFormat, ...) \
    do \
    { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, " %s:%d. " logmsgFormat, \
            __FILE__, __LINE__, ##__VA_ARGS__); \
        logger.log(buf); \
    } while (0); \

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, " %s:%d. " logmsgFormat, \
            __FILE__, __LINE__, ##__VA_ARGS__); \
        logger.log(buf); \
    } while (0); \

#define LOG_FAIAL(logmsgFormat, ...) \
    do \
    { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(FAIAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, " %s:%d. " logmsgFormat, \
            __FILE__, __LINE__, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    } while (0); \

// #define MUDEBUG 1 // 开启DEBUG

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, " %s:%d. " logmsgFormat, \
            __FILE__, __LINE__, ##__VA_ARGS__); \
        logger.log(buf); \
    } while (0) // 因为#else，为保证语法不需要加分号
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif
    

// 定义日志级别 INFO ERROR FAIAL DEBUG
// 枚举类型，这里面的数值只能是整数(默认int, 也可指定其他整数类型，float报错)
enum LogLevel{
    INFO,
    ERROR,
    FAIAL,// core dump
    DEBUG,
    NUMS_LOG_LEVEL,
};

// 输出一个日志类
class Logger : noncopyable // class默认私有继承，struct默认共有继承
{
public:
    // 获取日志的唯一实例
    static Logger& instance();
    // 设置日志级别
    int setLogLevel(int level);
    // 写日志
    void log(std::string msg);
private:
    int loglevel_;
    Logger(){}
};