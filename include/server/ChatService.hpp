#ifndef __CHATSERVICE_H
#define __CHATSERVICE_H

#include "json.hpp"
#include "public.hpp"
#include "UserModel.hpp"
#include "OfflineMessageModel.hpp"
#include "friendModel.hpp"
#include "GroupModel.hpp"
#include "redis.hpp"

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>

using namespace muduo;
using namespace muduo::net;
using namespace nlohmann;

// 消息对应的回调函数
using MsgHandler = 
    std::function<void(
        const TcpConnectionPtr&,    // tcp
        json&,                      // json
        Timestamp                   // time
    )>;

/////////////////////////////// 业务模块（单例模式）
class ChatService
{
public:
    // 获取单例对象
    static ChatService* instance();

    // 处理登录业务
    void login(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 处理注册业务
    void reg(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 点对点聊天业务
    void oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 创建群组业务
    void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 添加群组业务
    void addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 群组聊天业务
    void groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 注销业务
    void loginOut(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 获取消息对应的回调函数
    MsgHandler getMsgHandler(int msgType);

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr& conn);

    // 服务器异常，业务重置方法
    void reset();

    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int userid, const std::string& msg);

private:
    // 构造函数私有化
    ChatService();

    // 消息 --> 相应的回调函数
    std::unordered_map<int, MsgHandler> msgHandlerMap_;

    // 存储在线用户的通信连接（需要考虑线程安全）
    std::unordered_map<int, TcpConnectionPtr> userConnMap_;

    // 互斥锁
    std::mutex connMtx_;


    // 数据操作类对象
    UserModel userModel_;
    OfflineMsgModel offlineMsgModel_;
    FriendModel friendModel_;
    GroupModel groupModel_;

    // redis对象
    Redis redis_;
};

#endif