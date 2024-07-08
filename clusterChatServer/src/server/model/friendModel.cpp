#include "friendModel.hpp"
#include "db.h"

#include <vector>
using namespace std;


bool FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d, %d)", userid, friendid);
    
    MySQL mysql;   // 创建数据库对象
    if (mysql.connect()){
        if (mysql.update(sql)){
            return true;
        }
    }

    return false;
}


// User表和friend表的联合查询
Vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    // 在b表中查询用户userid的所有friendid，然后用此id去a表查所需信息
    sprintf(sql, "select a.id, a.name, a.state from user a inner join 
                    friend b on b.friendid=a.id where b.userid=%d", userid);

    vector<User> vec;
    MySQL mysql;
    if (mysql.connect()){

        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) // 查询成功
        {
            
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res) != nullptr))
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }

            mysql_free_result(res);
            return vec;
        }
    }

    // 返回默认构造，数值为-1
    return vec;
}
