#include "chatserver.hpp"
#include "chatservice.hpp"

#include <signal.h>
#include <iostream>

using namespace std;

// 处理服务器Ctrl+C退出后，重置用户的在线状态
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main()
{
    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop();

    return 0;
}