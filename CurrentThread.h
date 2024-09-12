#pragma once

namespace CurrentThread
{
    extern __thread int t_cachedTid;
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
