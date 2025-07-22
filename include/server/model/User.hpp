#ifndef __USER_H
#define __USER_H

#include <string>

// 数据库user表的ORM（Object Relation Map）类
class User 
{
public:
    // 构造函数
    User(
        int id = -1, 
        const std::string& name = "", 
        const std::string& pwd = "", 
        const std::string& state = "offline")
    : id_(id)
    , name_(name)
    , password_(pwd)
    , state_(state) {}

    // 析构函数
    ~User() {}

    // 设置id
    void setId(int id) {
        id_ = id;
    }

    // 设置名称
    void setName(const std::string& name) {
        name_ = name;
    }

    // 设置密码
    void setPwd(const std::string& pwd) {
        password_ = pwd;
    }

    // 设置状态
    void setState(const std::string& state) {
        state_ = state;
    }

    // 获取id
    int getId() {
        return id_;
    }

    // 获取名称
    std::string getName() {
        return name_;
    }

    // 获取密码
    std::string getPwd() {
        return password_;
    }

    // 获取状态
    std::string getState() {
        return state_;
    }

protected:
    int id_;
    std::string name_;
    std::string password_;
    std::string state_;
};

#endif