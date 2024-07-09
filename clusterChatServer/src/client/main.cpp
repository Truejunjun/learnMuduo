#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
using namespace std;
using json = nlohman::json;

#include <unistd.h>
#include <sys/socke.h>
#include <sys/types>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>  // 用于线程之间的信号量
#include <atomic>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"


// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息 
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;

// 控制主菜单界面的死循环
bool isMainMenuRunning = false;

// 用于读写线程之间的通信
sem_t rwsem;
// 记录登录状态
atomic_bool g_isLoginSuccess{false};


// 命令对应的处理函数
void help(int fd = 0, string str = "");
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void loginout(int, string);

// 接受线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
// 主聊天页面程序
void mainMenu();
// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令, 格式help"},
    {"chat", "一对一聊天, 格式chat:friendid:message"},
    {"addfriend", "添加好友, 格式addfriend:friendid"},
    {"creategroup", "创建群组, 格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组, 格式addgroup:groupid"},
    {"groupchat", "群聊, 格式groupchat:groupid:message"},
    {"loginout", "注销, loginout"},
};


// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout},
};

// 聊天客户端程序实现，main线程用作发送线程，子线程用作接受线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid!  Example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数输入的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写client需要连接的server信息ip和port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client与server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // 初始化读写线程通信信号量
    sem_init(&rwsem, 0, 0); // 前面的0是选功能，后一个0是初始设置为0
    
    // 连接服务器成功后，创建只读子线程
    std::thread readTask(readTaskHandler, clientfd);    // 相当于Linux的pthread_create
    readTask.detach();  // 相当于Linux的pthread_detach


    // main线程用于接受用户输入，负责发送数据(死循环)
    for (;;)
    {
        cout << "============" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. loginout" << endl;
        cout << "============" << endl;
        cout << "Plz choose" << endl;
        int choice = 0;
        cin >> choice;
        cin.get();  // 读掉缓冲区残留的回车

        switch (choice)
        {
            case 1: // login业务
            {
                int id = 0;
                char pwd[50] = {0};
                cout << "userid:";
                cin >> id;
                cin.get();  // 读掉缓冲区残留的回车
                cout << "userpassword:";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = password;
                string request = js.dump();

                g_isLoginSuccess = false;

                // 发送登录json信息
                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if (len == -1)
                {
                    cerr << "send login msg error: " << request << endl;
                }
                
                sem_wait(&rwsem);   // 等待子线程的信号量，子线程处理完登录响应后回此

                if (g_isLoginSuccess)
                {
                    isMainMenuRunning = true;
                    // 进入聊天主菜单
                    mainMenu();
                }
            }
            break;

            case 2: // register业务
            {
                char name[50] = {0};
                char pwd[50] = {0};
                cout << "username:";
                cin.getline(name, 50);  // 遇见回车才结束 
                cout << "userpassword:";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = password;
                string request = js.dump();

                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if (len == -1)
                {
                    cerr << "send reg msg error: " << request << endl;
                }

                sem_wait(&rwsem);   // 等待子线程的信号量，子线程处理完注册响应后回此
            }
            break;

            case 3: // loginout业务
                close(clientfd);
                sem_destroy(&rwsem);    // 释放信号量
                exit(0);

            default:
                cerr << "invalid input!" << endl;
                break;
        }
    }
    return 0;
}


// 处理注册的响应逻辑
void doRegResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>())
    {
        cerr << "This name is already exist, register error!" << endl;
    }
    else    // 注册成功
    {
        cout << "register success, userid is" << responsejs["id"] 
                << ", do not forget it." << endl;
    }
}


// 处理登录的响应逻辑
void doLoginResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>())    // 登录失败
    {
        cerr << responsejs["errmsg"] << endl;
        g_isLoginSuccess = false;
    }
    else    // 登录成功
    {
        // 记录当前用户的id和name
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);


        // 记录当前用户的好友列表信息
        if (responsejs.contains("friends"))
        {
            // 初始化，防止多次打印
            g_currentUserFriendList.clear();

            vector<string> vec = responsejs["friends"];
            for (string &str : vec)
            {
                json js = json::parse(str); // 反序列化
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }

        // 记录当前用户的群组列表信息 
        if (responsejs.contains("groups"))
        {
            // 初始化，防止多次打印
            g_currentUserGroupList.clear();

            vector<string> vec1 = responsejs["groups"];
            for (string &groupstr : vec1)
            {
                json grpjs = json::parse(groupstr);
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setState(grpjs["groupdesc"]);

                vector<string> vec2 = grpjs["users"];
                for (string  &userstr : vec2)
                {
                    GroupUser user;
                    json js = json::parse(userstr);
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"])
                    group.getUsers().push_back(user);
                }

                g_currentUserGroupList.push_back(group);                                    
            }
        }

        // 显示用户的基本信息
        showCurrentUserData();

        // 显示当前用户的离线信息， 个人和群组的
        if (responsejs.contains("offlinemsg"))
        {
            vector<string> vec = responsejs["offlinemsg"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                if (ONE_CHAT_MSG == js["msgid"].get<int>())
                {
                    cout << js["time"] << "[" << js["id"] << "]"  << js["name"] 
                    << "said:" << js["msg"] << endl;
                }
                else    // 群消息
                {
                    cout << "Group Message["<< js["groupid"] << "]:" << js["time"] <<
                    "[" << js["id"] << "]"  << js["name"] 
                    << "said:" << js["msg"] << endl;
                }
                
            }
        }

        g_isLoginSuccess = true;
    }
}



// 接收线程
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);  // 阻塞
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }

        // 接收chatServer转发的数据，反序列化成json
        json js = json::parse(buffer);
        int MSG_TYPE = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == MSG_TYPE)
        {
            cout << js["time"] << "[" << js["id"] << "]"  << js["name"] 
                << "said:" << js["msg"] << endl;
        }
        
        if (GROUP_CHAT_MSG == MSG_TYPE)
        {
            cout << "Group Message["<< js["groupid"] << "]:" << js["time"] <<
                 "[" << js["id"] << "]"  << js["name"] 
                 << "said:" << js["msg"] << endl;
            continue;
        }


        if (LOGIN_MSG_ACK == MSG_TYPE)
        {
            doLoginResponse(js);    // 处理登录响应的业务逻辑
            sem_post(&rwsem);   // 处理信号量，告知主线程，子线程已完成登录操作
            continue;
        }


        if (REG_MSG_ACK == MSG_TYPE)
        {
            doRegResponse(js);    // 处理登录响应的业务逻辑
            sem_post(&rwsem);   // 处理信号量，告知主线程，子线程已完成注册操作
            continue;
        }

    }
}


// 主聊天页面程序
void mainMenu(int clienfd)
{
    help();

    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; // 存储命令 
        int idx = commandbuf.find(":");
        if (-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }

        // 调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));  // 调用命令处理方法
    }
}


void help(int, string)
{
    cout << endl;
    cout << "show command list :" << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << ":" << p.second << endl;
    }
    cout << endl;
}

void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send add friends msg error: " << buffer << endl;
    }
}


void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send chat msg error: " << buffer << endl;
    }
}


void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "create group command invalid!" << endl;
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send create group msg error: " << buffer << endl;
    }
}



void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());

    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send add group msg error: " << buffer << endl;
    }
}


void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "group chat command invalid!" << endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send group chat msg error: " << buffer << endl;
    }

}


void loginout(int clientfd, string str)
{

    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send add group msg error: " << buffer << endl;
    }
    else    // 注销成功
    {
        isMainMenuRunning = false;
    }

}


void showCurrentUserData()
{
    cout << "======login user======" << endl;
    cout << "current login user => id :" << g_currentUser.getId() << "name: " << g_currentUser.getName() << endl;
    
    cout << "------Friend List------"
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    
    cout << "------Group List------"
        if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState() 
                     << " " << user.getRole() << endl;
            }
        }
    }

    cout << "==================" << endl;
}   