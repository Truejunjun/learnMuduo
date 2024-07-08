#ifndef USERMODEL_H
#define USERMODEL_H

#include <string>
#include "user.hpp"

// User表的操作类
class UserModel
{
public:
    // 增加User方法---当mysql完成创建之后才会获取到id填入User
    bool insert(User &user);

    // 根据用户号码查询用户信息
    User query(int id);

    // 更新用户的状态信息
    bool updateState(User user);

    // 重置用户的状态信息
    void resetState();
};

#endif