#ifndef __MYSQL_H
#define __MYSQL_H

#include <mysql/mysql.h>
#include <string>

class MySQL
{
public:
    // 构造函数
    MySQL();
    
    // 析构函数
    ~MySQL();

    // 连接数据库
    bool connect();

    // 更新数据库
    bool update(const std::string& sql);

    // 查询数据库
    MYSQL_RES* query(const std::string& sql);

    // 获取连接
    MYSQL* getConnection();

private:
    MYSQL* conn_;
};

#endif