#ifndef __DBCONN_HPP__
#define __DBCONN_HPP__

#include <mysql/mysql.h>
#include <string>
#include <chrono>

// 数据库操作类
class DataBaseConn
{
public:
    // 构造函数
    DataBaseConn();

    // 析构函数
    ~DataBaseConn();

    // 连接数据库
    bool connect(
        const std::string& ip,          // IP地址
        unsigned int port,              // 端口号 
        const std::string& user,        // 用户名
        const std::string& password,    // 密码
        const std::string& dbname       // 数据库名
    );

    // 更新操作
    bool update(const std::string& sql);

    // 查询操作
    MYSQL_RES* query(const std::string& sql);

    // 获取MySQL连接句柄
    MYSQL* getConnection() const;

    // 更新连接在连接池中的存活时间
    void updateAliveTime();

    // 获取连接在连接池中的存活时间（s）
    size_t getAliveTime() const;

private:
    MYSQL* conn_;                                       // MySQL连接句柄
    std::chrono::steady_clock::time_point timePoint_;   // 记录时间点
};

#endif