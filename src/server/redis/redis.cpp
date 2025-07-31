#include "redis.hpp"
#include "Logger.hpp"

#include <iostream>

// 构造函数
Redis::Redis() 
    : publish_context_(nullptr)
    , subscribe_context_(nullptr)
{}

// 析构函数
Redis::~Redis() {
    if(publish_context_ != nullptr) {
        redisFree(publish_context_);
    }

    if(subscribe_context_ != nullptr) {
        redisFree(subscribe_context_);
    }
}

// 连接redis服务器
bool Redis::connect() {
    // 负责publish发布消息的上下文连接
    publish_context_ = redisConnect("127.0.0.1", 6379);
    if(publish_context_ == nullptr) {
        // std::cerr << "[publish] connect redis failed!\n";
        LOG_ERROR() << "[publish] connect redis failed!";

        return false;
    }

    // 负责subscribe订阅消息的上下文连接
    subscribe_context_ = redisConnect("127.0.0.1", 6379);
    if(subscribe_context_ == nullptr) {
        // std::cerr << "[subscribe] connect redis failed!\n";
        LOG_ERROR() << "[publish] connect redis failed!";

        return false;
    }

    // 在单独线程中监听通道上的事件
    std::thread t(
        [&]() {
            observer_channel_message();
        }
    );

    t.detach();

    // std::cout << "connect redis-server success!\n";
    LOG_INFO() << "connect redis-server success";

    return true;
}

// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, const std::string& message) {
    // redisCommand是​​完全阻塞​​的：从发送命令到接收响应的全过程均阻塞当前线程
    // redisCommand功能 == redisAppendCommand + redisBufferWrite + redisGetReply功能
    // 显式拆分提供了**​流水线优化​​**和**​​阻塞控制​​**的灵活性
    redisReply* reply = (redisReply *)redisCommand(
        publish_context_, 
        "PUBLISH %d %s", 
        channel, 
        message.c_str()
    );

    if(reply == nullptr) {
        // std::cerr << "publish command failed!\n";
        LOG_ERROR() << "publish command failed!";

        return false;
    }

    freeReplyObject(reply);

    return true;
}

// 向redis指定的通道subscribe订阅消息（订阅消息后，当前的上下文会阻塞）
/*
    Redis订阅机制原理：
        - ​​阻塞发生在消息监听阶段​​：
            - 执行SUBSCRIBE后，连接进入​​订阅状态​​，需通过redisGetReply等函数循环读取消息。
              此阶段连接会阻塞，无法执行其他命令（如GET/SET）
        - 本函数仅发送订阅命令，将阻塞的消息监听移交至独立线程（observer_channel_message），符合异步设计最佳实践
*/
bool Redis::subscribe(int channel) {
    // SUBSCRIBE命令本身会造成线程阻塞等待通道中的消息
    /*
        向Redis指定频道（整数形式）发起订阅请求，​​仅负责发送订阅命令并确保命令写入Redis服务器​​，不处理实际消息接收。
        消息接收由独立线程中的observer_channel_message函数处理，避免阻塞主线程
    */
    // redisAppendCommand：非阻塞​​函数，仅将命令写入客户端的输出缓冲区后立即返回，​​不等待响应
    //// [ redisAppendCommand：命令 ----> 客户端的输出缓冲区 ]
    if(redisAppendCommand(subscribe_context_, "SUBSCRIBE %d", channel) == REDIS_ERR) {
        // std::cerr << "subscribe command failed!\n";
        LOG_ERROR() << "subscribe command failed!";

        return false;
    }

    // redisBufferWrite​​：将缓冲区中的**命令**​​同步发送至Redis服务器​​
    // redisBufferWrite可以循环发送缓冲区，直至缓冲区数据发送完毕
    //// [ redisBufferWrite：客户端的输出缓冲区 ----> Redis服务器​​ ]
    int done = 0;
    while(!done) {
        if(redisBufferWrite(subscribe_context_, &done) == REDIS_ERR) {
            // std::cerr << "subscribe command failed!\n";
            LOG_ERROR() << "subscribe command failed!";

            return false;
        }
    }

    return true;
}

// 向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel) {
    if(redisAppendCommand(subscribe_context_, "UNSUBSCRIBE %d", channel) == REDIS_ERR) {
        // std::cerr << "unsubscribe command failed!\n";
        LOG_ERROR() << "unsubscribe command failed!";

        return false;
    }

    int done = 0;
    while(!done) {
        if(redisBufferWrite(subscribe_context_, &done) == REDIS_ERR) {
            // std::cerr << "unsubscribe command failed!\n";
            LOG_ERROR() << "unsubscribe command failed!";

            return false;
        }
    }

    return true;
}

// 在独立线程中接收订阅通道中的消息
/*
    - 运行在​​独立线程​​中，负责​​持续监听​​Redis订阅通道的消息，
      并将消息通过回调函数notify_message_handler_上报给业务层。
      其核心是解决订阅阻塞与主线程解耦问题

    - 显式组合支持多次redisAppendCommand + 单次redisGetReply循环，​​减少网络往返次数​​，提升吞吐量
*/
void Redis::observer_channel_message() {
    redisReply* reply = nullptr;
    while(redisGetReply(subscribe_context_, (void **)&reply) == REDIS_OK) {
        /*
            Redis订阅消息是​​三元素数组
                - element[0]：消息类型
                - element[1]：频道名
                - element[2]：消息内容
        */
        if(reply != nullptr && 
           reply->element[2] != nullptr && 
           reply->element[2]->str != nullptr) 
        {
            // 将频道ID和消息内容传递给业务层处理函数notify_message_handler_，实现业务解耦
            notify_message_handler_(
                atoi(reply->element[1]->str), 
                reply->element[2]->str
            );
        }

        freeReplyObject(reply);
    }

    // std::cerr << "observer_channel_message quit\n";
    LOG_ERROR() << "observer_channel_message quit";
}

// 初始化向业务层上报通道消息的回调函数
void Redis::init_notify_handler(std::function<void(int, const std::string&)> fn) {
    notify_message_handler_ = fn;
}   