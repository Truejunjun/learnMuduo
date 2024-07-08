#ifndef GROUP_H
#define GROUP_H

#include <vector>
using namespace std;


class Group
{
public:
    Group(int id = -1, string name = "", string desc = "")
    {
        this->id = id;
        this->name = name;
        this->desc = desc;
    }

    // 设置
    void setId(int id)  { this->id = id; }
    void setName(string name)   { this->name = name; }
    void setDesc(string desc) { this->desc = desc; }

    // 读取
    int getId()  { return this->id; }
    string getName()   { return this->name; }
    string getDesc() { return this->desc; }

    vector<GroupUser> &getUsers()   { return this->users; }

private:
    int id;
    string name;
    string desc;
    vector<GroupUser> users;
};


#endif