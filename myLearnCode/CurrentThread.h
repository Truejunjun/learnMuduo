#pragma once
#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    extern __thread int t_cachedTId;

    void cachedTId();

    // 内联函数，只在当前文件起作用
    inline int tid()
    {
        if (__builtin_expect(t_cachedTId == 0, 0))  // 如果还没有获取过tid，就通过cachedTid获取
        {
            cachedTId();
        }
        return t_cachedTId; // 如果已经获取过了，直接从缓存中返回
    }
};