#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include <vector>
#include "user.hpp"
using namespace std;

// User表的操作类
class FriendModel
{
public:
    // 添加好友关系
    bool insert(int userid, int friendid);

    // 返回用户的好友列表
    Vector<User> query(int userid);
};

#endif