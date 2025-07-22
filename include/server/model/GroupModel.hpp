#ifndef __GROUPMODEL_H
#define __GROUPMODEL_H

#include "Group.hpp"
#include <vector>
#include <string>

// 数据库allgroup、groupuser操作类
class GroupModel
{
public:
    // 创建群组
    // 向数据库的allgroup表中增加群组信息
    bool createGroup(Group& group);

    // 加入群组
    void addGroup(int userid, int groupid, const std::string& role);

    // 查询用户所在的所有群组信息
    std::vector<Group> queryGroups(int userid);

    // 根据指定的groupid查询群组用户id列表（除userid自身）
    std::vector<int> queryGroupUsers(int userid, int groupid);
};

#endif