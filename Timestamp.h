#pragma once

#include <iostream>
#include <string>
#include <stdint.h>

class Timestamp
{
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    // 获取当前时间
    // 类中的静态成员
    static Timestamp now();
    // 标准化输出时间
    std::string toString() const;
private:
    int64_t microSecondsSinceEpoch_;
};
