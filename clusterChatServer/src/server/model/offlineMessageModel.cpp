#include "offlineMessageModel.hpp"
#include "db.h"


// 存储用户的离线消息
void OfflineMsgModel::insert(int userid, string msg)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d, '%s')", userid, msg.c_str());
    
    MySQL mysql;   // 创建数据库对象
    if (mysql.connect()){
        mysql.update(sql)
    }
}

// 删除用户的离线信息
void OfflineMsgModel::remove(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid=%d", userid);
    -+
    MySQL mysql;   // 创建数据库对象
    if (mysql.connect()){
        mysql.update(sql)
    }
}

// 查询用户的离线信息
vector<string> OfflineMsgModel::query(int userid)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);

    vector<string> vec;
    MySQL mysql;
    if (mysql.connect()){

        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) // 查询成功
        {
            
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res) != nullptr))
            {
                vec.push_back(row[0]);
            }

            mysql_free_result(res);
            return vec;
        }
    }

    // 返回默认构造，数值为-1
    return vec;
}