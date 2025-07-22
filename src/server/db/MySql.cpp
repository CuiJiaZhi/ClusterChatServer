#include "MySql.hpp"
#include <muduo/base/Logging.h>

using namespace muduo;

// 数据库配置信息
static std::string server = "127.0.0.1";
static std::string user = "root";
static std::string password = "123456";
static std::string dbname = "chat";

    // 构造函数
MySQL::MySQL() {
    conn_ = mysql_init(nullptr);
}
    
// 析构函数
MySQL::~MySQL() {
    if(conn_ != nullptr) {
        mysql_close(conn_);
    }
}

// 连接数据库
bool MySQL::connect() {
    MYSQL *p = mysql_real_connect(
        conn_, 
        server.c_str(), 
        user.c_str(),
        password.c_str(), 
        dbname.c_str(), 
        3306, 
        nullptr, 
        0
    );

    if(p != nullptr) {
        mysql_query(conn_, "set names gbk");

        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << "connect mysql success";
    }
    else {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << "connect mysql fail"; 
    }

    return p;
}

// 更新数据库
bool MySQL::update(const std::string& sql) {
    if(mysql_query(conn_, sql.c_str())) {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << " 更新失败!";

        return false;
    }

    return true;
}

// 查询数据库
MYSQL_RES* MySQL::query(const std::string& sql) {
    if(mysql_query(conn_, sql.c_str())) {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << " 查询失败!";
        
        return nullptr;
    }

    return mysql_use_result(conn_);
}

// 获取连接
MYSQL* MySQL::getConnection() {
    return conn_;
}