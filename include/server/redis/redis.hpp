#ifndef __REDIS_H
#define __REDIS_H

#include <hiredis/hiredis.h>
#include <functional>
#include <string>
#include <thread>

// redis操作类
class Redis
{
public:
    // 构造函数
    Redis();

    // 析构函数
    ~Redis();

    // 连接redis服务器
    bool connect();

    // 向redis指定的通道channel发布消息
    bool publish(int channel, const std::string& message);

    // 向redis指定的通道subscribe订阅消息（订阅消息后，当前的上下文会阻塞）
    bool subscribe(int channel);

    // 向redis指定的通道unsubscribe取消订阅消息
    bool unsubscribe(int channel);

    // 在独立线程中接收订阅通道中的消息
    void observer_channel_message();

    // 初始化向业务层上报通道消息的回调函数
    void init_notify_handler(std::function<void(int, const std::string&)> fn);

private:
    // hiredis同步上下文对象（redis-cli，客户端），负责publish消息
    redisContext* publish_context_;

    // hiredis同步上下文对象（redis-cli，客户端），负责subscribe消息
    redisContext* subscribe_context_;

    // 回调函数，收到订阅的消息，上报至ChatService层
    std::function<void(int, const std::string&)> notify_message_handler_;
};

#endif