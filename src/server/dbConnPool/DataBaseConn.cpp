#include "DataBaseConn.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <stdexcept>

// 构造函数
DataBaseConn::DataBaseConn() {
    // 初始化MySQL连接句柄
    conn_ = mysql_init(nullptr);
    if(conn_ == nullptr) {
        throw std::runtime_error("mysql_init() failed");
    }
}

// 析构函数
DataBaseConn::~DataBaseConn() {
    // 关闭MySQL连接
    if(conn_) {
        mysql_close(conn_);
    }
}

// 连接数据库
bool DataBaseConn::connect(
    const std::string& ip, 
    unsigned int port,
    const std::string& user,
    const std::string& password,
    const std::string& dbname
) 
{
    if(conn_ == nullptr) {
        throw std::runtime_error("MySQL connection is not initialized");
    }

    // 设置连接参数
    mysql_options(conn_, MYSQL_OPT_CONNECT_TIMEOUT, "5");

    // 连接数据库
    if(mysql_real_connect(conn_, ip.c_str(), user.c_str(), password.c_str(), dbname.c_str(), port, nullptr, 0) == nullptr) {
        return false;   // 连接失败
    }

    // 设置字符集
    mysql_query(conn_, "set names gbk");
    
    // 连接成功
    return true;
}

// 更新操作
bool DataBaseConn::update(const std::string& sql) {
    // 执行更新操作
    if(mysql_query(conn_, sql.c_str())) {
        // LOG("Update failed: " + sql);
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << "Update failed: " + sql;

        return false;   // 更新失败
    }

    return true;        // 更新成功
}

// 查询操作
MYSQL_RES* DataBaseConn::query(const std::string& sql) {
    // 执行查询操作
    if(mysql_query(conn_, sql.c_str())) {
        // LOG("Query failed: " + sql);
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << "Query failed: " + sql;

        return nullptr;   // 查询失败
    }

    return mysql_use_result(conn_);  // 返回查询结果
}

// 获取MySQL连接句柄
MYSQL* DataBaseConn::getConnection() const {
    if(conn_ == nullptr) {
        throw std::runtime_error("MySQL connection is not initialized");    
    }

    return conn_;  // 返回MySQL连接句柄
}

// 更新连接在连接池中的存活时间
void DataBaseConn::updateAliveTime() {
    // 更新记录时间点
    timePoint_ = std::chrono::steady_clock::now();
}

// 获取连接在连接池中的存活时间（s）
size_t DataBaseConn::getAliveTime() const {
    // 计算从记录时间点到现在的时间差
    auto duration = std::chrono::steady_clock::now() - timePoint_;
    
    // 返回时间差的秒数
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}
