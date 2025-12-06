/**
 *  muduo 网络库给用户提供了2个主要的类
 *  TcpServer：用于编写服务器
 *  TcpClient：用于编写客户端
 * 
 *  epoll + 线程池 = muduo 网络库
 *  好处： 能够将网络I/O代码和业务代码区分开
 *        只暴露 用户的连接和断开， 用户的可读写事件
 */

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>

using namespace std;
using namespace placeholders;

using namespace muduo;
using namespace muduo::net;


/**
 * 基于 muduo 网络库开发服务器程序
 * 
 * 1.组合 TcpServer 对象
 * 2.创建 EventLoop 事件循环对象的指针
 * 3.明确服务器的构造函数需要传递哪些参数
 * 4.在构造函数中注册处理连接的回调函数和处理读写事件的回调函数
 * 5.设置合适的服务器线程数量, muduo库会自己划分I/O线程和worker线程
 */
class ChatServer{
public:
    ChatServer(EventLoop* loop,                 //  事件循环
                const InetAddress& listenAddr,  // IP+Port
                const string& nameArg)          // 服务器名字
                : _server(loop, listenAddr, nameArg),
                 _loop(loop)
    {
        // 给服务器注册用户的连接和断开回调
        _server.setConnectionCallback(
            std::bind(&ChatServer::onConnection, this, _1) 
        );  // 最后的 _1 表示占位符，表示函数有一个参数


        // 给服务器注册用户读写时间的回调
        _server.setMessageCallback(
            std::bind(&ChatServer::onMessage, this, _1, _2, _3)
        );

        // 设置服务器端的线程数量
        _server.setThreadNum(4); // 1 个 I/O 线程， 3 个 worker 线程
    }

    // 开启事件循环
    void start(){
        _server.start();
    }

private:
    // 专门处理用户的连接创建和断开 
    void onConnection(const TcpConnectionPtr& conn){
        if(conn->connected()){
            // 打印对端和本端的ip和端口信息
            cout << conn->peerAddress().toIpPort() << " ->"
                << conn->localAddress().toIpPort() << " state:Online" << endl;
        }else{
            cout << conn->peerAddress().toIpPort() << " ->"
                << conn->localAddress().toIpPort() << " state:Offline" << endl;
            conn->shutdown();
        }
    }

    // 专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr& conn,    // 连接
                   Buffer* buffer,     // 缓冲区
                   Timestamp time)       // 接收到数据的时间信息
    {
        string buf = buffer->retrieveAllAsString(); // 获取缓冲区所有数据
        cout << "recv data:" << buf << "time:" << time.toString() << endl;
        conn->send(buf);
    }


    TcpServer _server; // #1
    EventLoop *_loop;  // #2 epoll 事件循环对象
};


int main(){
    EventLoop loop;
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop();  // epoll_wait 以阻塞方式等待新用户连接，已连接用户的读写事件等
    return 0;
}


// 可用 telnet 127.0.0.1 6000  测试服务器
// telnet 退出是 ctrl + ] 然后输入 quit