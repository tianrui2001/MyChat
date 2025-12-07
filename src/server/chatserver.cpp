#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"

#include <string>
#include <functional>

using namespace std::placeholders;
using namespace std;
using json = nlohmann::json;

// 初始化聊天室服务器
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg),
      _loop(loop)
{
    // 注册回调函数
    _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
    _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 上报连接相关的信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if(!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    string buf = buffer->retrieveAllAsString(); // 获取网络缓冲区中的数据
    json js = json::parse(buf);                 // 数据反序列化

    // 通过 js["msgid"] 获取消息ID -> 业务handler -> 调用handler(conn, js, time)
    // 完全解耦 网络模块 的代码和 业务模块 的代码
    ChatService::instance()->getHandler(js["msgid"].get<int>())(conn, js, time);

}