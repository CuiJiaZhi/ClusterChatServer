#ifndef __CHATSERVER_H
#define __CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;

/////////////////////////////// 网络模块
// 聊天服务器的主类
/*
    EventLoop: Reactor模式的核心类，负责事件循环，通过I/O多路复用监听文件描述符事件，并调用回调函数，其设计遵循"One Loop Per Thread"原则
    TcpServer: muduo库的服务端核心引擎，通过主从Reactor架构实现高并发
        - 主Reactor（Main Loop）：运行在主线程，由EventLoop实例管理，通过监听建立新连接并分发给子线程
        - 从Reactor(Sub Loop)：运行在I/O线程（子线程），每个线程独立运行EventLoop，处理已经建立连接的读写事件，通过TcpConnection管理单个连接的生命周期
        - 五大回调函数，项目中只是用两个回调函数
            - ​​ConnectionCallback​​：连接建立/断开时触发
            - MessageCallback​​：数据到达inputBuffer_（接收缓冲区）后触发，传递Buffer*和接收时间戳
            - 另外三个：
                - WriteCompleteCallback：outputBuffer_（发送缓冲区）清空时通知应用层可继续发送
                - ​​HighWaterMarkCallback​​：发送缓冲区超过阈值时触发流量控制（如暂停读取）
                - ​​CloseCallback​​（内部）：连接关闭时由TcpServer调用，用于移除连接管理
    TcpConnection: 管理单个TCP连接，封装了连接生命周期管理、数据读写、事件回调等核心功能
        - 通过状态机管理、双缓冲设置以及事件回调机制，实现高效可靠的TCP连接管理
            - 双缓存机制
                - inputBuffer_（接收缓冲区）：存储从​​Socket读取的接收数据​​，供应用层解析 
                    - TCP是无边界的字节流协议，数据可能分多次到达（如粘包、半包问题）
                    - 当数据不完整时，需暂存到inputBuffer_，待构成完整消息再通过MessageCallback通知业务逻辑
                - outputBuffer（发送缓冲区）：暂存​​待发送的应用层数据​​，逐步写入Socket
                - 两者均基于std::vector<char>实现，通过readerIndex_和 writerIndex_​​划分三个区域：
                    - ​​预留区​​（Prependable）：头部8字节，用于添加协议头等元数据
                    - ​​可读区​​（Readable）：存储待处理数据（inputBuffer_）或待发送数据（outputBuffer_）
                    - 可写区​​（Writable）：用于追加新数据
        - 单线程归属：所有操作必须在所属EventLoop线程执行，因严格单线程操作，成员变量无需加锁，避免竞争
*/
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
    // 用户连接/断开时触发的回调函数
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