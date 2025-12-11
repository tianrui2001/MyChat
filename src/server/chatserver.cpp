#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"
#include "public.hpp"

#include <string>
#include <functional>
#include <muduo/base/Logging.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

    // // UDP 心跳检测
    // // _UdpServer 初始化
    // _udpFd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);
    // if(_udpFd < 0){
    //     LOG_FATAL << "Create UDP socket failed!";
    // }

    // // 绑定端口
    // struct sockaddr_in addr;
    // bzero(&addr, sizeof addr);
    // addr.sin_family = AF_INET;
    // addr.sin_port = htons(listenAddr.port());
    // addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //  int ret = bind(_udpFd, (struct sockaddr *)&addr, sizeof addr);
    // if (ret < 0) {
    //     LOG_FATAL << "Bind UDP socket failed!";
    // }

    // // 创建 Channel 并注册到 Loop
    // _udpChannel.reset(new Channel(loop, _udpFd));
    
    // // 设置读回调 (当 UDP 有数据来时，调用 handleUdpRead)
    // _udpChannel->setReadCallback(bind(&ChatServer::handleUdpRead, this));
    
    // // 开启读监听
    // _udpChannel->enableReading();
    // LOG_INFO << "UDP Heartbeat Listener started on port " << listenAddr.port();

    //  // 注册定时器：每 10 秒执行一次 checkClientStatus
    // _loop->runEvery(10.0, bind(&ChatServer::checkClientStatus, this));

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
    else{
        // 如果一个连接在30秒内不发心跳，照样会被踢掉。
        conn->setContext(Timestamp::now());
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    // 只要 TCP 有数据来，也视为一次心跳，给由于 UDP 丢包的情况兜底
    conn->setContext(time);

    string buf = buffer->retrieveAllAsString(); // 获取网络缓冲区中的数据
    json js = json::parse(buf);                 // 数据反序列化

    // 2. 拦截心跳包，不往下走业务逻辑
    if (js.contains("msgid") && js["msgid"].get<int>() == (int)MsgId::HEARTBEAT_MSG) {
        return; 
    }

    // 通过 js["msgid"] 获取消息ID -> 业务handler -> 调用handler(conn, js, time)
    // 完全解耦 网络模块 的代码和 业务模块 的代码
    ChatService::instance()->getHandler(js["msgid"].get<int>())(conn, js, time);

}

// void ChatServer::handleUdpRead()
// {
//     char buf[1024] = {0};
//     struct sockaddr_in clientAddr;
//     socklen_t len = sizeof(clientAddr);
    
//     ssize_t n = recvfrom(_udpFd, buf, sizeof(buf) - 1, 0, 
//                            (struct sockaddr*)&clientAddr, &len);

//     if (n > 0)
//     {
//         try {
//             json js = json::parse(buf);
//             ChatService::instance()->handleHeartbeat(js, Timestamp::now());
//         } catch (...) {
//             LOG_ERROR << "UDP Heartbeat parse error: " << buf;
//         }
//     }
// }

void ChatServer::checkClientStatus()
{
    Timestamp now = Timestamp::now();
    ChatService::instance()->checkClientStatus(now);
}