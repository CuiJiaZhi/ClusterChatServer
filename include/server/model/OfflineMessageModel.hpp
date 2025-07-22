#ifndef __OFFLINEMESSAGEMODEL_H
#define __OFFLINEMESSAGEMODEL_H

#include <string>
#include <vector>

// 数据库中离线消息表的操作类
class OfflineMsgModel
{
public:
    // 存储用户的离线消息
    void insert(int userid, const std::string& msg);

    // 删除用户的离线消息
    void remove(int userid);

    // 查询用户离线消息
    std::vector<std::string> query(int userid);
};

#endif