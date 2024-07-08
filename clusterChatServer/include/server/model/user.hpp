#ifndef USR_H
#define USR_H

#include <string>

// 匹配User表的ORM类
class User
{
public:
    User(int id=-1, string name="", string pwd="", string state="offline")
    {
        this->id = id;
        this->name = name;
        this->password = password;
        this->state = state;
    }

    // 设置
    void setId(int id)  { this->id = id; }
    void setName(string name)   { this->name = name; }
    void setPassword(string password)    { this->password = password; }
    void setState(string state) { this->state = state; }

    // 读取
    int getId()  { return this->id; }
    string getName()   { return this->name; }
    string getPassword()    { return this->password; }
    string getState() { return this->state; }

private:
    int id;
    string name;
    string password;
    string state;
};

#endif