#ifndef __CHATSERVER_H
#define __CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;

/////////////////////////////// 网络模块
// 聊天服务器的主类
class ChatServer
{
public:
    // 构造函数
    ChatServer(EventLoop* evLoop, 
               const InetAddress& listenAddr, 
               const std::string& nameArg);

    // 启动
    void start();

private:
    // 处理用户连接的回调函数
    void onConnection(const TcpConnectionPtr& conn);

    // 处理读写事件的回调函数
    void onMessage(const TcpConnectionPtr& conn, 
                   Buffer* buffer, 
                   Timestamp time);

private:
    TcpServer server_;    // 
    EventLoop* evLoop_;   // 事件循环
};

#endif