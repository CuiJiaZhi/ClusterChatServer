#include "ChatService.hpp"
#include <muduo/base/Logging.h>
#include <vector>

using namespace muduo;

/////////////////////////////// 业务模块
// 获取单例对象
ChatService* ChatService::instance() {
    static ChatService service;
    
    return &service;
}

// 构造函数
// 注册消息的回调函数
ChatService::ChatService() {
    // 注册登录消息的回调函数
    msgHandlerMap_.insert(
        { MSG_LOGIN, 
          std::bind(&ChatService::login, this, _1, _2, _3) }
    );

    // 注册注册消息的回调函数
    msgHandlerMap_.insert(
        { MSG_REG, 
          std::bind(&ChatService::reg, this, _1, _2, _3) }
    );

    // 注册点对点聊天业务回调函数
    msgHandlerMap_.insert(
        { MSG_ONE_CHAT, 
          std::bind(&ChatService::oneChat, this, _1, _2, _3) }
    );

    // 注册添加添加好友业务回调函数
    msgHandlerMap_.insert(
        { MSG_ADD_FRIEND, 
          std::bind(&ChatService::addFriend, this, _1, _2, _3) }
    );

    // 注册创建群组业务回调函数
    msgHandlerMap_.insert(
        { MSG_CREATE_GROUP, 
          std::bind(&ChatService::createGroup, this, _1, _2, _3) }
    );

    // 注册添加群组业务回调函数
    msgHandlerMap_.insert(
        { MSG_ADD_GROUP, 
          std::bind(&ChatService::addGroup, this, _1, _2, _3) }
    );

    // 注册群组聊天业务回调函数
    msgHandlerMap_.insert(
        { MSG_GROUP_CHAT, 
          std::bind(&ChatService::groupChat, this, _1, _2, _3) }
    );

    // 注册注销业务回调函数
    msgHandlerMap_.insert(
        { MSG_LOGINOUT, 
          std::bind(&ChatService::loginOut, this, _1, _2, _3) }
    );

    // 连接redis服务器
    if(redis_.connect()) {
        // 设置上报消息的回调
        redis_.init_notify_handler(
            std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2)
        );
    }
}

// 处理登录业务
void ChatService::login(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    // 获取待登录的用户ID
    int id = js["id"].get<int>();

    // 获取待登录用户的密码
    std::string pwd = js["password"];

    User user = userModel_.query(id);

    if(user.getId() == id && user.getPwd() == pwd) {
        if(user.getState() == "online") {
            // 该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = MSG_LOGIN_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using";

            conn->send(response.dump());
        }
        else {
            // 登录成功，给客户端返回消息
            // 记录用户连接信息
            {
                std::lock_guard<std::mutex> lock(connMtx_);
                userConnMap_.insert({ id, conn });
            }

            // 登录成功，向redis订阅channel(id)
            redis_.subscribe(id);

            // 更新用户状态
            user.setState("online");
            userModel_.updateState(user);

            json response;
            response["msgid"] = MSG_LOGIN_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询该用户是否有离线消息并返回
            std::vector<std::string> offlineMsgVec = offlineMsgModel_.query(id);
            if(!offlineMsgVec.empty()) {
                // 转发该用户的离线消息
                response["offlinemsg"] = offlineMsgVec;
                
                // 从数据库中将该用户的离线消息删除
                offlineMsgModel_.remove(id);
            }

            // 查询该用户的好友信息并返回
            std::vector<User> friendVec = friendModel_.query(id);
            if(!friendVec.empty()) {
                std::vector<std::string> temp;
                for(auto&& f : friendVec) {
                    json js;
                    js["id"] = f.getId();
                    js["name"] = f.getName();
                    js["state"] = f.getState();
                    temp.push_back(js.dump());
                }

                response["friends"] = temp;
            }

            // 查询用户的群组信息
            std::vector<Group> groupuserVec = groupModel_.queryGroups(id);
            if(!groupuserVec.empty()) {
                // group: [{ groupid:[xxx, xxx, xxx, xxx] }]
                std::vector<std::string> groupV;
                for(auto&& group : groupuserVec) {
                    // 遍历每个群组
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();

                    std::vector<std::string> userV;
                    for(auto&& user : group.getUsers()) {
                        // 遍历每个群组中的群成员
                        json userjs;
                        userjs["id"] = user.getId();
                        userjs["name"] = user.getName();
                        userjs["state"] = user.getState();
                        userjs["role"] = user.getRole();

                        userV.push_back(userjs.dump());
                    }

                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }

            conn->send(response.dump());
        }
    }
    else {
        // 登录失败
        json response;
        response["msgid"] = MSG_LOGIN_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";

        conn->send(response.dump());
    }
}

// 处理注册业务
void ChatService::reg(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    // 获取待注册的用户名
    std::string name = js["name"];

    // 获取待注册用户的密码
    std::string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    if(userModel_.insert(user)) {
        // 注册成功
        json response;
        response["msgid"] = MSG_REG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();

        conn->send(response.dump());
    }
    else {
        // 注册失败
        json response;
        response["msgid"] = MSG_REG_ACK;
        response["errno"] = 1;

        conn->send(response.dump());
    }
}

// 点对点聊天业务
void ChatService::oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    // 对端（B）id
    int toid = js["toid"].get<int>();

    {
        // 寻找对端id
        std::lock_guard<std::mutex> lock(connMtx_);
        auto it = userConnMap_.find(toid);
        if(it != userConnMap_.end()) {
            // 对端在线，转发消息
            // A和B在同一台服务器中且均处于在线状态
            // 服务器将A发给B的消息转发给B，服务器当作一个中转站
            it->second->send(js.dump());

            return;
        }
    }

    // A和B不在同一台服务器或者A和B在同一台服务器但是B不在线
    // 查询toid是否在线
    User user = userModel_.query(toid);
    if(user.getState() == "online") {
        // 表示A和B不在同一台服务器
        // 发布消息
        redis_.publish(toid, js.dump());

        return;
    }

    // 对端（B）不在线，存储离线消息
    offlineMsgModel_.insert(toid, js.dump());
}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    friendModel_.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    int userid = js["id"].get<int>();
    std::string name = js["groupname"];
    std::string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if(groupModel_.createGroup(group)) {
        // 存储群组创建人信息
        groupModel_.addGroup(userid, group.getId(), "creator");
    }
}

// 添加群组业务
void ChatService::addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    groupModel_.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    std::vector<int> useridVec = groupModel_.queryGroupUsers(userid, groupid);

    std::lock_guard<std::mutex> lock(connMtx_);
    for(int id : useridVec) {
        auto it = userConnMap_.find(id);
        if(it != userConnMap_.end()) {
            // 转发群消息
            it->second->send(js.dump());
        }
        else {
            // 查询对端是否在线
            User user = userModel_.query(id);
            if(user.getState() == "online") {
                redis_.publish(id, js.dump());
            }
            else {
                // 存储离线群消息
                offlineMsgModel_.insert(id, js.dump());
            }
        }
    }
}

// 注销业务
void ChatService::loginOut(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    int userid = js["id"].get<int>();

    {
        std::lock_guard<std::mutex> lock(connMtx_);
        auto it = userConnMap_.find(userid);
        if(it != userConnMap_.end()) {
            userConnMap_.erase(it);
        }
    }

    // 用户注销（下线），在redis中取消订阅
    redis_.unsubscribe(userid);

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    userModel_.updateState(user);
}

// 获取消息对应的回调函数
MsgHandler ChatService::getMsgHandler(int msgType) {
    auto it = msgHandlerMap_.find(msgType);
    if(it == msgHandlerMap_.end()) {
        // 默认回调函数
        return [&](const TcpConnectionPtr&, json&, Timestamp){
            LOG_ERROR << "msgid: " << msgType << " can not find handler!";
        };
    }
    else {
        return msgHandlerMap_[msgType];
    }
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr& conn) {
    User user;
    {
        std::lock_guard<std::mutex> lock(connMtx_);
        // 根据value查找key
        for(auto it = userConnMap_.begin(); it != userConnMap_.end(); ++it) {
            if(it->second == conn) {
                //删除用户的连接信息
                user.setId(it->first);
                userConnMap_.erase(it);
                break;
            }
        }
    }

    // 用户注销（下线），在redis中取消订阅
    redis_.unsubscribe(user.getId());

    // 更新用户的状态信息
    if(user.getId() != -1) {
        user.setState("offline");
        userModel_.updateState(user);
    }
}

// 服务器异常，业务重置方法
void ChatService::reset() {
    // 将所有online状态的用户设置为offline
    userModel_.resetState();
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, const std::string& msg) {
    std::lock_guard<std::mutex> lock(connMtx_);
    auto it = userConnMap_.find(userid);
    if(it != userConnMap_.end()) {
        it->second->send(msg);

        return;
    }   

    // 存储该用户的离线消息
    offlineMsgModel_.insert(userid, msg);
}