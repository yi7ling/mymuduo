#pragma once

namespace CurrentThread
{
    extern __thread int t_cachedTid;
    void cachedTid();

    inline int tid()
    {
        // 编译器优化
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cachedTid();
        }
        return t_cachedTid;
    }
}
