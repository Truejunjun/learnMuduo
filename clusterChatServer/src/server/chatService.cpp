#include "chatService.hpp"
#include "public.hpp"
#include <string>
#include <vector>
#include <muduo/base/Logging.h>
using namespace muduo;
using namespace placeholders;
using namespace std;

ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

ChatService::ChatService()
{
    // 将业务服务绑定
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login(), this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::login(), this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat(), this, _1, _2, _3)});
}


// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR << "msgid:" << msgid << "not found";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}


// 处理登录和注册业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 目前还是用的id进行登录
    int id = js["id"].get<int>();
    string pwd = js["password"];

    // 通过查询id找到user(user为空就会等于-1)，并核对密码
    User user = _userModel.query(id);
    if (user.getId() != -1 && user.getPassword() == pwd)
    {
        // 已经登上了
        if (user.getState() == "online")
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;  // 有错
            response["errmsg"] = "该账号已登录";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功，记录用户链接信息
            {
                lock_guard<mutex> lock(_connMutex); // 采用智能指针lock_guard加锁，作用到第一个右花括号
                _userConnMap.insert({id, conn});
            }
            // 登录成功，更新用户状态信息
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;  // 无错，成功
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 还要查询是否存在离线时传来的信息
            vector<string> offlineMsgVec = _offlineMsgModel.query(id);
            if (!offlineMsgVec.empty())
            {
                response["offlinemsg"] = vec;
                _offlineMsgModel.remove(id);    // 把离线消息缓存删掉
            }

            conn->send(response.dump());
        }
    }
    else
    {
        // 登录失败 --- 1. 用户不存在  2. 密码错误 
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;  // 无错，成功
        conn->send(response.dump());
    }
}


// 注册业务，只需要输入name 和 password即可
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPassword(pwd);
    bool state = _userModel.insert(user);
    if (state){
        // 注册成功 
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;  // 无错，成功
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else{
        // 注册失败 
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;  // 失败
        response["id"] = user.getId();
        conn->send(response.dump());
    }
}


// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);        
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 从map中删除用户的链接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    if (user.getId() != -1){    // 找得到才处理，找不到就不用处理了
        // 更新下线信息
        user.setState("offline");
        _UserModel.updateState(user);
    }
}


// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["to"].get<int.();

    // 查看对方是否在线 和 通过conn进行发送都需要上锁保护
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // 对方在线
            it->second->send(js.dump);  // 服务器直接推送消息给toid用户，conn->send()
            return;
        }
    }

    // 用户不在线
    _offlineMsgModel.insert(toid, js.dump());
}


// 服务器异常，将用户状态都设置成offline
void ChatService::reset()
{
    _userModel.resetState();
}