#pragma once

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>

#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

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

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    // 处理添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 服务器异常退出，业务重置方法
    void reset();

    // 处理创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理注销业务
    void logout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理redis订阅消息
    void handleRedisSubscribeMessage(int userid, string message);

    // // 处理心跳包
    // void handleHeartbeat(json &js, Timestamp time);

    //检查客户端状态，关闭超时连接
    void checkClientStatus(Timestamp time);

private:
    ChatService(); // 单例模式

    // 存储消息id和对应的业务处理方法
    unordered_map< int, MsgHandler> _msghandlerMap;

    // 存储在线通信连接
    unordered_map< int, TcpConnectionPtr> _userConnMap;
    mutex _connMutex;

    // 数据操作类对象
    UserModel       _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel     _friendModel;
    GroupModel      _groupModel;

    // redis操作对象
    Redis          _redis;
};