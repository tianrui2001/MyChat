#include "chatservice.hpp"
#include "public.hpp"

#include <muduo/base/Logging.h>
#include <functional> // std::bind 需要
using namespace std::placeholders;

using namespace muduo;

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
            user.setState("online");
            _userModel.updateState(user);   // 更新用户状态

            json response;
            response["msgid"] = MsgId::LOGIN_MSG_ACK;
            response["id"] = user.getId();
            response["name"] = user.getName();  // 一般存客户端本地
            response["errno"] = 0;
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