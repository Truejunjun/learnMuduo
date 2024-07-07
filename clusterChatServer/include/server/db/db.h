#ifndef DB_H
#define DB_H

#include <muduo/base/Logging.h>
#include <mysql/mysql.h>
#include <string>
using namespace std;


// 数据库信息
static string server = "IP";
static string user = "root";
static string password = "123456";
static string dbname = "chat";


// 数据库操作类
class MySQL
{
public:
    // 构造函数初始化数据库链接
    MySQL() { _conn = mysql_init(nullptr); }
    ~MySQL(){
        if (_conn != nullptr){
            mysql_close(_conn);
        }
    }

    // 链接数据库
    bool connect()
    {
        // 3306是端口号
        MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str()
                        password.c_str(), dbname.c_str(), 3306, nullptr, 0)

        if (p != nullptr)
        {
            // c和C++代码默认的字符是ACSII，所以拉取的中文会乱码
            mysql_query(_conn, "set names gbk");
            LOG_INFO << "connected mysql.";
        }
        else{
            LOG_INFO << "failed.";
        }
        return p;
    }

    // 更新操作
    bool update(string sql)
    {
        if (mysql_query(_conn, sql.c_str()))
        {
            LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "更新失败";
            return false;
        }
        return true;
    }

    // 查询操作
    MYSQL_RES *query(string sql)
    {
        if (mysql_query(_conn, sql.c_str()))
        {
            LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "查询失败";
            return nullptr;
        }
        return mysql_use_result(_conn);
    }

    // 获取私有链接
    MYSQL* getConnection()  { return _conn; }
    
private:
    MYSQL *_conn;
};

#endif