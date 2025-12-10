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

int main(int argc, char **argv)
{
    if(argc < 3){
        cout << "invalide input! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(0);
    }

    string ip = argv[1];
    uint16_t port = atoi(argv[2]);


    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop();

    return 0;
}
