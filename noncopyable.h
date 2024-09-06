#pragma once

/**
 * 继承noncopyable的类，派生类对象可以进行 默认构造和析构，但不能
 * 进行 拷贝构造和赋值操作
 */
class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;

    // 区别移动构造函数 noncopyable(noncopyable&&)
    noncopyable& operator=(const noncopyable&) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};