#pragma once

namespace CurrentThread
{
    extern __thread int t_cachedTid;// 外部声明，必须有地方定义了
    void cachedTid();

    inline int tid()
    {
        if (__builtin_expect(t_cachedTid == 0, 0)) // 编译器优化, inexpect = 不期望
        {
            cachedTid();
        }
        return t_cachedTid;
    }
}
