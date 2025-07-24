#include <iostream>
#include <signal.h>

#include "ChatServer.hpp"
#include "ChatService.hpp"

// Ctrl + C信号处理函数
void resetHandler(int) {
    // 重置
    ChatService::instance()->reset();

    // 退出进程
    exit(0);
}

// ChatServer：网络层
// ChatService：业务层
// model/以及dp/：数据层
int main(int argc, char** argv)
{
    if(argc < 3) {
        std::cerr << "command invalid! example: ./server 127.0.0.1 6000\n";

        exit(-1);
    }

    signal(SIGINT, resetHandler);

    char* ip = argv[1];             // IP地址
    int port = atoi(argv[2]);       // 端口

    EventLoop evLoop;
    InetAddress addr(ip, port);
    ChatServer server(&evLoop, addr, "ChatServer");

    server.start();

    evLoop.loop();

    return 0;
}