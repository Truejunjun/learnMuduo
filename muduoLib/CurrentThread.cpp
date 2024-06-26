#include "CurrentThread.h"

namespace CurrentThread
{
    __thread int t_cachedTId = 0;

    void cacheTid()
    {
        if (t_cachedTId == 0)
        {
            // 通过Linux系统调用获取当前的线程号
            t_cachedTId = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}