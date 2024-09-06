#include "Timestamp.h"

#include <time.h>
#include <stdio.h>

Timestamp::Timestamp()
    : microSecondsSinceEpoch_(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

// 获取当前时间 调用linux库中的时间类，获取当前时间
Timestamp Timestamp::now()
{
    return Timestamp(time(NULL));
}

// 标准化输出时间 把序列化时间 转换为 标准化时间（年月日时分秒）
std::string Timestamp::toString() const
{
    char buf[128] = {0};
    tm *tm_time = localtime(&microSecondsSinceEpoch_);

    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
        tm_time->tm_year + 1900,
        tm_time->tm_mon + 1,
        tm_time->tm_mday,
        tm_time->tm_hour,
        tm_time->tm_min,
        tm_time->tm_sec
        );

    return buf;
}

/**
 * vscode远程连接linux，两种编译方式
 * 1. 终端g++ -o 编译
 * 2. vscode 构建tasks，使用gdb
 */

// #include <iostream>

// int main(void)
// {
//     std::cout << Timestamp::now().toString() << std::endl;

//     return 0;
// }
