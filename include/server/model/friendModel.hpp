#ifndef __FRIENDMODEL_H
#define __FRIENDMODEL_H

#include "User.hpp"
#include <vector>

// 数据库中好友表的操作类
class FriendModel
{
public:
    // 用户添加好友
    void insert(int userid, int friendid);

    // 返回用户好友列表（好友表 + 用户表联合查询）
    std::vector<User> query(int userid);
};

#endif