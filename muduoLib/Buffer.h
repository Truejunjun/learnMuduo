#pragma once

#include <string>
#include <vector>
#include <algorithm>

///	|	prependable bytes	|	readable bytes	| 	writable bytes	|
///	|						|					|					|
///	|						|	   CONTENT	 	|					|
///	0		<=			readerIndex		<=	writerIndex		<=		size

// 网络库底层的缓冲区类型定义
class Buffer
{
public:
	static const size_t kCheapPrepend = 8;	// 记录数据的长度
	static const size_t	kInitialSize = 1024;// 缓冲区初始的大小
	
	explicit Buffer(size_t initialSize = kInitialSize)
		: buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend),
		writerIndex_(kCheapPrepend)
    {}
    
    size_t readableBytes() const	{ return writerIndex_ - readerIndex_;}
    // 整个buffer的长度
    size_t writableBytes() const	{ return buffer_.size() - writerIndex_;}
    size_t prependabelBytes() const { return readerIndex_;}
    
    // 返回缓冲区可读数据的首地址
    const char* peek() const	{ return begin() + readerIndex_;}
    
    void retrieve(size_t len)
    {
    	if (len < readableBytes())
    	{
    		rederIndex_ += len;	// 应用只读了一部分，还剩下readerIndex+=len → writerIndex_没读取
    		
    	}
    	else
    	{
    		retrieveAll();
    	}
    }
    
    void retrieveAll()
    {
    	// 将可读和可写都拉到开头的8位之后
    	rederIndex_ = writerIndex_ = kCheapPrepend;
    }
    
    // 把onMessage函数上报的Buffer数据，转成string类型返回
    std::string retrieveAllAsString()
    {
    	return retrieveAsString(readableBytes());	// 应用可读数据的长度
    }
    
    std::string retrieveAsString(size_t len)
    {
    	std::string result(peek(), len);
    	retrieve(len);	// 上一句把缓冲区的可读数据读取出来，这里肯定要对缓冲区数据进行复位操作
    	return result;
    }
    
    // 确保可写区间够长
    void ensureWriteableBytes(size_t len)
    {
    	if (writableBytes() < len)
    	{
    		makeSpace(len);	// 扩容函数
    	}
    }
    
    // 把[data, data+len]内存上的数据，添加到writable缓冲区中
    void append(const char* data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }

    char* beginWrite()  { return begin() + witerIndex_; }
    const char* beginWrite() const { return begin() + witerIndex_; }

    // 从fd上读取数据，读到可写空间
    ssize_t readFd(int fd, int* saveErrno);
	// 写入数据到fd，从可读空间读
	ssize_t writeFd(int fd, int* saveErrno);
    
private:
	// 取vector数组首元素的地址
	char* begin()	{	return &*buffer_.begin(); }
	const char* begin const { return &*buffer_.begin(); }
	
    void makeSpace(size_t len)
    {
    	// 8字节+已读空闲长度+可写长度 < 8字节+扩容长度
    	if (prependableBytes() + writableBytes() < len + kCheapPrepend)
    	{
    		buffer_.resize(witerIndex_ + len);	// 直接在可写里扩容
    	}
    	else
		{
			// 将可读数据往前搬，把空闲和可写拼起来;把A到B之间的copy到C起点
			size_t readable = readableBytes();
			std::copy(begin()+readerIndex_,
						begin()+writerIndex_,
						begin()+kCheapPrepend);
            readerIndx_ = kCheapPrepend;
            writerIndex_ = readerIndx_ + readable;
		}
    }

	std::vector<char> buffer_;
	size_t readerIndex_;
	size_t writerIndex_;
};