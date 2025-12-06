#pragma once

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>

#include "json.hpp"
#include "usermodel.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

// 定义消息处理器类型
using MsgHandler = function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

// 聊天服务器业务类
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService* instance();

    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

private:
    ChatService(); // 单例模式

    // 存储消息id和对应的业务处理方法
    unordered_map< int, MsgHandler> _msghandlerMap;

    // 数据操作类对象
    UserModel _userModel;
};