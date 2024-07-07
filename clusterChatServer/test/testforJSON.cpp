#include "json.hpp"

using json = nlohman::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;

string func1()
{
    json js;
    js["msg_type"] = 2;
    js["form"] = "zhang 3";
    js["to"] = "Li 4";
    js["msg"] = "damn.";

    string sendBuf = js.dump;   // 将信息序列化转成字符串，通过网络发送
    cout << sendBuf.c_str() << endl;
    // cout << js << endl;  # 可以直接这种输出

    return sendBuf;
}

void func2()
{
    json js;
    js["id"] = {1,2,3};
    js["name"] = "zhang 3";

    js["msg"]["zhang 3"] = "damn.";
    js["msg"]["Li 4"] = "shit.";
    js["msg"] = {{"zhang 3", "damn."}, {"Li 4", "shit."}};  // 等同于上两句
}

void func3()
{
    json js;

    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    js["list"] = vec;

    map<int, string> m;
    m.insert({1, "x"});
    m.insert({2, "y"});
    m.insert({3, "z"});

    js["path"] = m;
    string sendBuf = js.dump;   // 将信息序列化转成字符串，通过网络发送

}



int main()
{
    // 从网络中获取到序列化的信息
    string recvBuf = func1();
    // 开始反序列化 json字符串 → 反序列化到数据对象
    json jsbuf = json::parse(recvBuf);
    return 0;
}