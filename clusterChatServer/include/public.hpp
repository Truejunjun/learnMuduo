#ifndef PUBLIC_H
#define PUBLIC_H


// 往下没定义的，系统会自动按顺序递增赋值，也就是REG_MSG_ACK=3
enum EnMsgType
{
    LOGIN_MSG = 1,  // 登录业务的消息id
    LOGIN_MSG_ACK,  // 登陆响应消息
    REG_MSG,        // 注册业务的消息
    REG_MSG_ACK,    // 注册响应消息
    ONE_CHAT_MSG,   // 一对一聊天消息
    ADD_FRIEND_MSG, // 添加好友消息

    CREATE_GROUP_MSG,   // 创建群组
    ADD_GROUP_MSG,      // 加入群组
    GROUP_CHAT_MSG,     // 群聊天

    LOGINOUT_MSG,     // 注销业务的消息
};

#endif