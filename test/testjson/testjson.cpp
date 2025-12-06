#include "json.hpp"

using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>

using namespace std;

// json 序列化示例
void func1(){
    json js;
    js["msg_type"] = 2;     // json 就是key-value 形式的对象
    js["from"] = "zhangsan";
    js["to"] = "lisi";
    js["msg"] = "hello, china";

    // json 转 字符串
    string str = js.dump();
    // cout << str.c_str() << endl; // 网络发送的是 char* 类型的数据

    cout << js << endl;
}   // 无序map 序列化示例

void func2(){
    json js;
    json["id"] = {1,2,3,4,5};   // 数组
    json["name"] = "zhangsan";
    json["msg"]["zhangsan"] = "hello world"; // 嵌套 json 对象
    json["msg"]["lisi"] = "hello zhangsan";
    json["msg"] = {{"zhangsan", "hello world"}, {"lisi", "hello zhangsan"}};

    cout << js << endl;
}

void func3(){   // 容器序列化
    json js;
    vector<int> vec = {1, 2, 5};
    js["list"] = vec;

    map<int, string> m = {
        {1, "zhangsan"},
        {2, "lisi"},
        {3, "wangwu"}
    };

    js["map"] = m;
    cout << js << endl;

    string str = js.dump();
    cout << str << endl;

}


string func4(){
    json js;
    js["msg_type"] = 2;     // json 就是key-value 形式的对象
    js["from"] = "zhangsan";
    js["to"] = "lisi";
    js["msg"] = "hello, china";

    // json 转 字符串
    string str = js.dump();
    return str;
} 

int main(){
    func1();
    func2();
    func3();

    // 数据序列化
    string recvBuf = func4();
    // 数据反序列化
    json jsbuf = json::parse(recvBuf);
    cout << jsbuf["msg_type"] << endl;
    cout << jsbuf["from"] << endl;
    cout << jsbuf["to"] << endl;
    cout << jsbuf["msg"] << endl;

    return 0;
}