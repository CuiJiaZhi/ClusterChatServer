#include "ChatServer.hpp"
#include "ChatService.hpp"
#include "json.hpp"

#include <functional>
#include <string>

using namespace std::placeholders;
using namespace nlohmann;

/////////////////////////////// 网络模块
// 构造函数
ChatServer::ChatServer(
    EventLoop* evLoop, 
    const InetAddress& listenAddr, 
    const std::string& nameArg)
    : server_(evLoop, listenAddr, nameArg)
    , evLoop_(evLoop)
{
    // 处理用户连接的回调函数
    server_.setConnectionCallback(
        std::bind(&ChatServer::onConnection, this, _1)
    );

    // 处理读写事件的回调函数
    server_.setMessageCallback(
        std::bind(&ChatServer::onMessage, this, _1, _2, _3)
    );

    // 设置线程数
    server_.setThreadNum(4);
}

// 启动服务
void ChatServer::start() {
    server_.start();
}

// 处理用户连接的回调函数
void ChatServer::onConnection(const TcpConnectionPtr& conn) {
    if(!conn->connected()) {
        // 客户端断开连接
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 处理读写事件的回调函数
void ChatServer::onMessage(
    const TcpConnectionPtr& conn, 
    Buffer* buffer,
    Timestamp time) 
{
    // 从缓冲区读取数据
    std::string buf = buffer->retrieveAllAsString();

    // 数据的反序列化
    json js = json::parse(buf);

    //// 解耦网络模块与业务模块
    // 绑定回调函数，根据消息ID确定要进行的相应处理
    MsgHandler msgHandler = ChatService::instance()->getMsgHandler(js["msgid"].get<int>());
    // 执行回调函数
    msgHandler(conn, js, time);
}