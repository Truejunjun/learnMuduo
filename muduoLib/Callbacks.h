#pragma once

#include <memeory>
#include <funcitonal>

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void (const TcpConnectionPtr&)>;
using CloseCallback = std::function<void (const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void (const TcpConnectionPtr&)>;
using HighWaterMarkCallback = std::funciton<void (const TcpConnectionPtr&, size_t)>;    // 收发数据的速率阈值

using MessageCallback = std::function<void (const TcpConnectionPtr&, 
                                            Buffer*, Timestamp)>;