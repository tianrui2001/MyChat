#include "chatservice.hpp"
#include "public.hpp"
#include "offlinemodel.hpp"
#include "friendmodel.hpp"

#include <vector>
#include <muduo/base/Logging.h>
#include <functional> // std::bind 需要
using namespace std::placeholders;

using namespace muduo;
using namespace std;

ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

ChatService::ChatService()
{
    // 注册消息以及对应的handler
    _msghandlerMap.insert({(int)MsgId::LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msghandlerMap.insert({(int)MsgId::REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msghandlerMap.insert({(int)MsgId::ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msghandlerMap.insert({(int)MsgId::ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
}


void ChatService::reset()
{
    // 把所有在线用户的状态设置为offline
    _userModel.resetState();
}

// 处理登录业务 id + password
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"];
    string pwd = js["password"];

    User user = _userModel.query(id);
    if(user.getId() != -1 &&  user.getPassword() == pwd){
        if(user.getState() == "online"){
            // 该用户已经登录
            json response;
            response["msgid"] = MsgId::LOGIN_MSG_ACK;
            response["errno"] = 2; // 已经登录的错误
            response["errmsg"] = "该用户已经登录, 请勿重复登录!";
            conn->send(response.dump());
        }
        else{
            // 登录成功
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn}); // 存储用户连接信息
            }
            
            user.setState("online");
            _userModel.updateState(user);   // 更新用户状态

            json response;
            response["msgid"] = MsgId::LOGIN_MSG_ACK;
            response["id"] = user.getId();
            response["name"] = user.getName();  // 一般存客户端本地
            response["errno"] = 0;

            // 查询是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if(!vec.empty()){
                response["offlinemsg"] = vec;
                // 删除离线消息
                _offlineMsgModel.remove(id);
            }

            // 查询好友列表
            vector<User> userVec = _friendModel.query(id);
            if(!userVec.empty()){
                 vector<string> friendVec;
                 for(User &user : userVec){
                    json temp;
                    temp["id"] = user.getId();
                    temp["name"] = user.getName();
                    temp["state"] = user.getState();
                    friendVec.push_back(temp.dump());
                 }
                 response["friends"] = friendVec;
            }

            conn->send(response.dump());
        }
    }
    else{
        // 用户名或者密码错误
        json response;
        response["msgid"] = MsgId::LOGIN_MSG_ACK;
        response["errno"] = 1;  // 用户名或者密码错误
        response["errmsg"] = "用户名或密码错误!";
        conn->send(response.dump());
    }
}

void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPassword(pwd);
    bool state = _userModel.insert(user);
    if(state){
        // 注册成功
        json response;
        response["msgid"] = MsgId::REG_MSG_ACK;
        response["id"] = user.getId();
        response["errno"] = 0;
        conn->send(response.dump());
    }else{
        // 注册失败
        json response;
        response["msgid"] = MsgId::REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msghandlerMap.find(msgid);
    if(it == _msghandlerMap.end()){
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time){ // 按值获取的lamda表达式
            LOG_ERROR << "msgid:" << msgid << " can not find handler!"; // 不用加endl
        };
    }
    else{
        return _msghandlerMap[msgid];
    }
    
}


void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["to"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end()){
            // toid 在线, 转发消息
            it->second->send(js.dump());
            return;
        }
    }

    // toid 不在线，存储离线消息，待登录的时候发送给对方
    _offlineMsgModel.insert(toid, js.dump());

}


void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        // 更新用户的连接信息
        lock_guard<mutex> lock(_connMutex);
        for(auto it = _userConnMap.begin(); it != _userConnMap.end(); it++){
            if(it->second == conn){
                _userConnMap.erase(it);
                user.setId(it->first);
                break;
            }
        }
    }
    
    // 更新用户的状态信息
    if(user.getId() != -1){
        user.setState("offline");
        _userModel.updateState(user);
    }
}

void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 将好友关系写入数据库
    _friendModel.insert(userid, friendid);
}


void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string groupname = js["groupname"];
    string groupdesc = js["groupdesc"];

    Group group(-1, groupname, groupdesc);
    if(_groupModel.createGroup(group)){
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    _groupModel.addGroup(userid, groupid, "normal");
}

void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for(int id : useridVec){
        if(_userConnMap.find(id) != _userConnMap.end()){
            _userConnMap[id]->send(js.dump());
        }else{
            _offlineMsgModel.insert(id, js.dump());
        }
    }

}