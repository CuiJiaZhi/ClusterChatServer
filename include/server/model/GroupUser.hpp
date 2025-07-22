#ifndef __GROUPUSER_H
#define __GROUPUSER_H

#include "User.hpp"
#include <string>

// 数据库groupuser表的ORM（Object Relation Map）类
class GroupUser : public User
{
public:
    // 构造函数
    GroupUser(
        int id = -1, 
        const std::string& name = "", 
        const std::string& pwd = "", 
        const std::string& state = "offline", 
        const std::string& role = "normal")
        : User(id, name, pwd, state)
        , role_(role)
    {}

    // 
    void setRole(const std::string& role) {
        role_ = role;
    }

    // 
    std::string getRole() {
        return role_;
    }

private:
    std::string role_;
};

#endif