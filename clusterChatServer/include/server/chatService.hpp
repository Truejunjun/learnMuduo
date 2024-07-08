#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>

#include "json.hpp"
#include "userModel.hpp"
#include "offLineMessageModel.hpp"
#include "friendModel.hpp"
#include "groupModel.hpp"

using namespace std;
using namespcace muduo;
using namespcace muduo::net;
using json = nlohman::json;

using MsgHandler = std::fucntion<void(const TcpConnectionPtr &conn, json &js, Timestamp)>


// 聊天服务器业务类----单例即可
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService* instance();

    // 处理登录和注册业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 创建群组
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注销业务
    void loginOut(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 获取消息对应的处理器
    MsgHandler getHandler(int);

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    // 服务器异常，业务重置处理
    void reset();
    
private:
    ChatService();  // 采用单例，将构造函数放入私有，并暴露一个指针方法

    unordered_map<int, MsgHandler> _msgHandlerMap;  // 消息id和业务处理方法

    unordered_map<int, TcpConnectionPtr> _userConnMap;  // 存储在线用户的通信链接，与线程有关

    // 数据操作类对象，提供的都是数据库的对象，而不是字段
    UserModel _userModel;                   // 普通user的数据库操作函数
    OfflineMsgModel _offlineMsgModel;       // 离线消息的数据库操作函数
    FriendModel _friendModel;               // 好友相关的数据库操作函数
    GroupModel _groupModel;                 // 群组相关的数据库操作函数

    mutex _connMutex;   // 定义互斥锁，保证_userConnMap的线程安全
};

#endif