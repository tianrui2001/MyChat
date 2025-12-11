#include "json.hpp"

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <ctime>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

using namespace std;
using json = nlohmann::json;
using namespace std::chrono;

sem_t rwsem;
atomic<bool> g_isLoginSuccess{false};   // 标记登录是否成功

User g_currentUser;
vector<User> g_currentUserFriendList;
vector<Group> g_currentGroupList;
bool isMainMenuRunning = false;

void logProc(int clientfd);
void regProc(int clientfd);
void showCurrentUserData();
void readTaskHandler(int clientfd);
void mainMenu(int clientfd);
string getCurrentTime();
void doLoginResponse(json &response);
void doRegResponse(json &response);

void help(int clientfd, string);
void chat(int clientfd, string);
void addFriend(int clientfd, string);
void createGroup(int clientfd, string);
void addGroup(int clientfd, string);
void groupChat(int clientfd, string);
void logout(int clientfd, string);

unordered_map<string, string> commandMap = {
    {"help", "展示所有支持的命令"},
    {"chat", "一对一聊天，命令格式：chat:friendid:message"},
    {"addfriend", "添加好友，命令格式：addfriend:friendid"},
    {"creategroup", "创建群组，命令格式：creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，命令格式：addgroup:groupid"},
    {"groupchat", "群聊，命令格式：groupchat:groupid:message"},
    {"logout", "注销登录"}
};

unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addFriend},
    {"creategroup", createGroup},
    {"addgroup", addGroup},
    {"groupchat", groupChat},
    {"logout", logout}
};


int main(int argc, char **argv){
    if(argc < 3){
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建客户端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd == -1){
        cerr << "socket create error!" << endl;
        close(clientfd);
        exit(-1);
    }

    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // 连接服务器
    if(connect(clientfd, (sockaddr*)&server, sizeof(sockaddr_in)) == -1){
        cerr << "connect server error!" << endl;
        close(clientfd);
        exit(-1);
    }

    sem_init(&rwsem, 0, 0);

    // 连接成功，启动子线程接收消息
    std::thread readTask(readTaskHandler, clientfd);
    readTask.detach();

    while(true){
        // 显示首页面菜单 登录、注册、退出
        cout << "========================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "========================" << endl;
        int choice = 0;
        cin >> choice;
        cin.get(); // 处理掉缓冲区残留的回车!!!

        switch (choice)
        {
        case 1:
            logProc(clientfd);
            break;

        case 2:
            regProc(clientfd);
            break;

        case 3:
            /* code */
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
        
        default:
            break;
        }
    }

}

void regProc(int clientfd)
{
    char name[50] = {0};
    char password[50] = {0};
    cout << "username: ";
    cin.getline(name, 50);
    cout << "password: ";
    cin.getline(password, 50);

    json js;
    js["msgid"] = MsgId::REG_MSG;
    js["name"] = name;
    js["password"] = password;
    string req = js.dump();

    // 发送登录数据包
    int len = send(clientfd, req.c_str(), req.size()+1, 0);
    if(len == -1){  // 发送失败
        cerr << "send login msg error:" << req << endl;
     }

     sem_wait(&rwsem); // 等待子线程处理注册响应
}

void logProc(int clientfd)
{
    int id = 0;
    char password[50] = {0};
    cout << "userid: ";
    cin >> id;
    cin.get();
    cout << "password: ";
    cin.getline(password, 50);
            
    json js;
    js["msgid"] = MsgId::LOGIN_MSG;
    js["id"] = id;
    js["password"] = password;
    string req = js.dump();

    g_isLoginSuccess = false;

    int len = send(clientfd, req.c_str(), req.size()+1, 0);
    if(len == -1){  // 发送失败
        cerr << "send login msg error:" << req << endl;
    }

    sem_wait(&rwsem); // 等待子线程处理登录响应

    if(g_isLoginSuccess){
        // 进入聊天主菜单页面
        isMainMenuRunning = true;
        mainMenu(clientfd);
    }
}

void showCurrentUserData()
{
    cout << "========================" << endl;
    cout << "current user id: " << g_currentUser.getId() << endl;
    cout << "current user name: " << g_currentUser.getName() << endl;

    cout << "------------------------" << endl;
    cout << "friend list:" << endl;
    for(User &user : g_currentUserFriendList){
        cout << user.getId() << "\t" << user.getName() << "\t" << user.getState() << endl;
    }

    cout << "------------------------" << endl;
    cout << "group list:" << endl;
    for(Group &group : g_currentGroupList){
        cout << group.getId() << "\t" << group.getName() << "\t" << group.getDesc() << endl;
        cout << "group members:" << endl;
        for(GroupUser &user : group.getUsers()){
            cout << user.getId() << "\t" << user.getName() << "\t" 
                 << user.getState() << "\t" << user.getRole() << endl;
        }
    }
    cout << "========================" << endl;
}

void doLoginResponse(json &response)
{
    g_currentGroupList.clear();
    g_currentUserFriendList.clear();

    if(response["errno"].get<int>() == 0){
        // 登录成功
        g_currentUser.setId(response["id"].get<int>());
        g_currentUser.setName(response["name"]);
        cout << "login success, welcome " << g_currentUser.getName() << " !" << endl;

        if(response.contains("friends")){
            vector<string> friendArr = response["friends"];
            for(string &str : friendArr){
                json friendjs = json::parse(str);
                User user;
                user.setId(friendjs["id"]);
                user.setName(friendjs["name"]);
                user.setState(friendjs["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }

        if(response.contains("groups")){
            vector<string> groupArr = response["groups"];
            for(string &str : groupArr){
                json grpjs = json::parse(str);
                Group group;
                group.setId(grpjs["id"]);
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                vector<string> userArr = grpjs["users"];
                for(string &userstr : userArr){
                    json userjs = json::parse(userstr);
                    GroupUser user;
                    user.setId(userjs["id"]);
                    user.setName(userjs["name"]);
                    user.setState(userjs["state"]);
                    user.setRole(userjs["grouprole"]);
                    group.getUsers().push_back(user);
                }
                g_currentGroupList.push_back(group);
            }
        }

        showCurrentUserData();

        if(response.contains("offlinemsg")){
            vector<string> offlinemsgArr = response["offlinemsg"];
            cout << "you have " << offlinemsgArr.size() << " offline messages:" << endl;
            for(string &str : offlinemsgArr){
                json offlinemsgjs = json::parse(str);
                if(MsgId::ONE_CHAT_MSG == offlinemsgjs["msgid"]){
                    cout << offlinemsgjs["time"].get<string>() << " [" << offlinemsgjs["id"].get<int>() << "] "
                        << offlinemsgjs["name"].get<string>() << " said: " << offlinemsgjs["msg"].get<string>() << endl;
                }else if(MsgId::GROUP_CHAT_MSG == offlinemsgjs["msgid"]){
                    cout << "群消息[" << offlinemsgjs["groupid"].get<int>() << "]" 
                        << offlinemsgjs["time"].get<string>() << " [" << offlinemsgjs["id"].get<int>() << "] "
                        << offlinemsgjs["name"].get<string>() << " said: " << offlinemsgjs["msg"].get<string>() << endl;
                    continue;
                }
            }
        }
        g_isLoginSuccess = true;
    }
    else{
        // 登录失败
        cerr << "login failed! reason: " << response["errmsg"] << endl;
        g_isLoginSuccess = false;
    }

}

void doRegResponse(json &response)
{
    if(response["errno"].get<int>() == 0){
        // 注册成功
        cout << "register success, your id is " << response["id"] 
            << ", please remember it!" << endl;
    }else{
        // 注册失败
        cerr << "register failed! reason: " << response["errmsg"] << endl;
    }
}


void readTaskHandler(int clientfd)
{
    while(true){
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, sizeof(buffer), 0);
        if(len == -1 || len == 0){
            cerr << "recv msg error!" << endl;
            close(clientfd);
            exit(-1);
        }

        // 接收 ChatServer 转发的消息， 反序列化成json数据对象
        json js = json::parse(buffer);
        MsgId msgtype = js["msgid"];

        switch (msgtype)
        {
            case MsgId::ONE_CHAT_MSG:
                cout << js["time"].get<string>() << " [" << js["id"].get<int>() << "] "
                    << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
                break;

            case MsgId::GROUP_CHAT_MSG:
                cout << "群消息[" << js["groupid"].get<int>() << "]" 
                    << js["time"].get<string>() << " [" << js["id"].get<int>() << "] "
                    << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
                break;

            case MsgId::LOGIN_MSG_ACK:
                doLoginResponse(js);
                sem_post(&rwsem);
                break;

            case MsgId::REG_MSG_ACK:
                doRegResponse(js);
                sem_post(&rwsem);
                break;

            default:
                break;
        }
    }
}

string getCurrentTime()
{
    auto tt = system_clock::to_time_t(system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

void mainMenu(int clientfd)
{
    help(clientfd, "");

    char buff[1024] = {0};
    while(isMainMenuRunning){
        cout << "main menu >> ";
        cin.getline(buff, 1024);
        string commandbuf(buff);
        string command; // 存储命令
        int index = commandbuf.find(":");   // 查找“：”的位置
        if(index == -1){
            command = commandbuf;
        }else{
            command = commandbuf.substr(0, index);
        }

        auto it = commandHandlerMap.find(command);
        if(it == commandHandlerMap.end()){
            cerr << "command not found!" << endl;
            continue;
        }

        it->second(clientfd, commandbuf.substr(index+1, commandbuf.size() - index));

    }

}



void help(int clientfd, string)
{
    cout << "show all command list:" << endl;
    for(auto &p : commandMap){
        cout << p.first << "\t" << p.second << endl;
    }

    cout << endl;
}

void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if(idx == -1){
        cerr << "command invalid!" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size()-idx);

    json js;
    js["msgid"] = MsgId::ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["to"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();

    string req = js.dump(); 
    int len = send(clientfd, req.c_str(), req.size()+1, 0);
    if(len == -1){
        cerr << "send chat msg error: " << req << endl;
    }
}

void addFriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());

    json js;
    js["msgid"] = MsgId::ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;

    string req = js.dump();
    int len = send(clientfd, req.c_str(), req.size()+1, 0);
    if(len == -1){
        cerr << "send addfriend msg error: " << req << endl;
    }
}

void createGroup(int clientfd, string str)
{
    int idx = str.find(":");
    if(idx == -1){
        cerr << "command invalid!" << endl;
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = MsgId::CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;

    string req = js.dump();
    int len = send(clientfd, req.c_str(), req.size()+1, 0);
    if(len == -1){
        cerr << "send createGroup msg error: " << req << endl;
    }
}

void addGroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());

    json js;
    js["msgid"] = MsgId::ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;

    string req = js.dump();
    int len = send(clientfd, req.c_str(), req.size()+1, 0);
    if(len == -1){
        cerr << "send addGroup msg error: " << req << endl;
    }
}

void groupChat(int clientfd, string str)
{
    int idx = str.find(":");
    if(idx == -1){
        cerr << "command invalid!" << endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = MsgId::GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();

    string req = js.dump();
    int len = send(clientfd, req.c_str(), req.size()+1, 0);
    if(len == -1){
        cerr << "send groupChat msg error: " << req << endl;
    }
}

void logout(int clientfd, string)
{
    int id = g_currentUser.getId();

    json js;
    js["msgid"] = MsgId::LOG_OUT_MSG;
    js["id"] = id;

    string req = js.dump();
    int len = send(clientfd, req.c_str(), req.size()+1, 0);
    if(len == -1){
        cerr << "send logout msg error: " << req << endl;
    }else{
        isMainMenuRunning = false;
        cout << "you have logged out!" << endl;
    }
}