#include "CurrentThread.h"

#include <sched.h>
#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    __thread int t_cachedTid = 0;

    void cachedTid()
    {
        if (t_cachedTid == 0)
        {
            // 通过系统调用，获取线程号
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}
