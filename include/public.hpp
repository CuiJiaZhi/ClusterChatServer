#ifndef __PUBLIC_H
#define __PUBLIC_H

// 消息类型
enum MsgType {
    MSG_LOGIN = 1,    // 登录消息
    MSG_LOGIN_ACK,    // 登录响应消息
    MSG_LOGINOUT,     // 注销消息
    MSG_REG,          // 注册消息
    MSG_REG_ACK,      // 注册响应消息
    MSG_ONE_CHAT,     // 点对点聊天
    MSG_ADD_FRIEND,   // 添加好友消息
    MSG_CREATE_GROUP, // 创建群组
    MSG_ADD_GROUP,    // 加入群组
    MSG_GROUP_CHAT,   // 群聊天
};

#endif