#include "Buffer.h"

#include <sys/uio.h>
#include <errno.h>
#include <unistd.h>

ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    char extrabuf[65536] = {0}; // 栈上的内存空间,64k
    struct iovec vec[2];

    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    // 如果上面的缓冲区大小不足才会开辟在这里填入，并且最终缓冲区的总长度就是数据的长度，不浪费一点资源
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const sszie_t n = ::ready(fd, vec, iovcnt);
    if ( n < 0 )
    {
        *saveErrno = errno;
    }
    else if (n <= writable) // 原buffer缓冲区就足够存储
    {
        writerIndex_ += n;
    }
    else    // extrabuf中也写入了数据
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }

    return n;   // 返回读取到的字节数
}

ssize_t Buffer::writeFd(int fd, int* saveErrno)
{
    sszie_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}