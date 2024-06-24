#pragma once

#include <memeory>
#include <funcitonal>

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void (const TcpConnectionPtr&)>;
using CloseCallback = std::function<void (const TcpConnectionPtr&)>;
using writeCompleteCallback = std::function<void (const TcpConnectionPtr&)>;


using MessageCallback() = std::function<void (const TcpConnectionPtr&, 
                                                    Buffer*, Timestamp)>;