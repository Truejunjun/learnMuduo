#include "userModel.hpp"
#include "db.h"
#include <iostream>

using namespace std;

// 增加User方法，但这里还是不关注业务层的逻辑
bool UserModel::insert(User &user)
{
    // 1. 组装sql语句 --- 刚开始没注册有user时，拿到的都是构造函数中的默认值
    // 使用c_str()将string转成char*类型
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
        user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());
    
    
    MySQL mysql;   // 创建数据库对象
    if (mysql.connect()){
        if (mysql.update(sql)){
            // 获取插入成功的用户数据生成的id
            user.setId(mysql_insert_id(mysql.getConnection()));

            return true;
        }
    }

    return false;
}


User UserModel::query(int id)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    MySQL mysql;
    if (mysql.connect()){

        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) // 查询成功
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));   // 通过atoi转成整型
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);

                mysql_free_result(res); // 释放内存资源
                return user;
            }
        }
    }

    // 返回默认构造，数值为-1
    return User();

}


bool UserModel::updateState(User user)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d", 
                    user.getState().c_str(), user.getId());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}


// 重置用户的状态信息
void UserModel::resetState()
{
    // 组装sql语句
    char sql[1024] = "update user set state = 'offline' where state = 'online'";

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}