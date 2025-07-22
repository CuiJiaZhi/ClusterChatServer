#ifndef __GROUP_H
#define __GROUP_H

#include "GroupUser.hpp"
#include <string>
#include <vector>

// 数据库allgroup表的ORM（Object Relation Map）类
// GroupUser：中间表
class Group
{
public:
    // 构造函数
    Group(int id = -1, const std::string& name = "", const std::string& desc = "")
        : id_(id)
        , name_(name)
        , desc_(desc)
    {}

    // 析构函数
    ~Group() {}

    // 
    void setId(int id) {
        id_ = id;
    }

    // 
    void setName(const std::string& name) {
        name_ = name;
    }

    // 
    void setDesc(const std::string& desc) {
        desc_ = desc;
    }

    // 
    int getId() {
        return id_;
    }

    // 
    std::string getName() {
        return name_;
    }

    // 
    std::string getDesc() {
        return desc_;
    }

    // 
    std::vector<GroupUser>& getUsers() {
        return user_;
    }

private:
    int id_;
    std::string name_;
    std::string desc_;
    std::vector<GroupUser> user_;
};

#endif