#include "OfflineMessageModel.hpp"
// #include "MySql.hpp"
#include "DataBaseConnPool.hpp"

// 存储用户的离线消息
void OfflineMsgModel::insert(int userid, const std::string& msg) {
    // 组装sql语句
    char sql[1024] = { 0 };
    sprintf(sql, 
        "insert into offlinemessage values('%d', '%s')", 
        userid, msg.c_str()
    );

    // MySQL mysql;
    // if(mysql.connect()) {
    //     mysql.update(sql);
    // }

    // 获取连接池实例
    DataBaseConnPool* pool = DataBaseConnPool::instance();
    if(pool != nullptr) {
        // 获取连接
        std::shared_ptr<DataBaseConn> conn = pool->getConnection();
        if(conn != nullptr) {
            // 执行插入操作
            conn->update(sql);
        }
    }
}

// 删除用户的离线消息
void OfflineMsgModel::remove(int userid) {
    // 组装sql语句
    char sql[1024] = { 0 };
    sprintf(sql, 
        "delete from offlinemessage where userid = %d", 
        userid
    );

    // MySQL mysql;
    // if(mysql.connect()) {
    //     mysql.update(sql);
    // }

    // 获取连接池实例
    DataBaseConnPool* pool = DataBaseConnPool::instance();
    if(pool != nullptr) {
        // 获取连接
        std::shared_ptr<DataBaseConn> conn = pool->getConnection();
        if(conn != nullptr) {
            // 执行删除操作
            conn->update(sql);
        }
    }
}

// 查询用户离线消息
std::vector<std::string> OfflineMsgModel::query(int userid) {
    // 组装sql语句
    char sql[1024] = { 0 };
    sprintf(sql, 
        "select message from offlinemessage where userid = %d", 
        userid
    );

    // std::vector<std::string> vec;
    // MySQL mysql;
    // if(mysql.connect()) {
    //     MYSQL_RES* res = mysql.query(sql);
    //     if(res != nullptr) {
    //         MYSQL_ROW row;
    //         while((row = mysql_fetch_row(res)) != nullptr) {
    //             vec.push_back(row[0]);
    //         }

    //         mysql_free_result(res);
    //     }
    // }

    std::vector<std::string> vec;
    // 获取连接池实例
    DataBaseConnPool* pool = DataBaseConnPool::instance();
    if(pool != nullptr) {
        // 获取连接
        std::shared_ptr<DataBaseConn> conn = pool->getConnection();
        if(conn != nullptr) {
            // 执行查询操作
            MYSQL_RES* res = conn->query(sql);
            if(res != nullptr) {
                MYSQL_ROW row;
                while((row = mysql_fetch_row(res)) != nullptr) {
                    vec.push_back(row[0]);
                }
                mysql_free_result(res);
            }
        }
    }

    return vec;
}