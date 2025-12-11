# pragma once

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Channel.h>


using namespace muduo;
using namespace muduo::net;

// 聊天服务器类
class ChatServer
{
public:
    ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg);
    void start();


private:
    void onConnection(const TcpConnectionPtr &conn);
    void onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time);

    // // 处理 UDP 心跳包
    // void handleUdpRead();

    // 检查客户端状态，关闭超时连接
    void checkClientStatus();

    TcpServer _server;
    EventLoop *_loop;

    // // 心跳检测
    // int _udpFd;
    // std::unique_ptr<Channel> _udpChannel;
};