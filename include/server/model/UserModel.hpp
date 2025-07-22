#ifndef __USERMODEL_H
#define __USERMODEL_H

#include "User.hpp"

// User的操作类
class UserModel
{
public:
    // 向User表中添加内容
    bool insert(User& user);

    // 根据用户号查询用户信息
    User query(int id);

    // 更新用户状态信息
    bool updateState(User& user);

    // 重置用户状态信息
    void resetState();
};

#endif