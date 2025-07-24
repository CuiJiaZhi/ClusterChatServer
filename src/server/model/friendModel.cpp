#include "friendModel.hpp"
// #include "MySql.hpp"
#include "DataBaseConnPool.hpp"

// 用户添加好友
void FriendModel::insert(int userid, int friendid) {
    // 组装sql语句
    char sql[1024] = { 0 };
    sprintf(sql, 
        "insert into friend values(%d, %d)", 
        userid, friendid
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

// 返回用户好友列表（好友表 + 用户表联合查询）
std::vector<User> FriendModel::query(int userid) {
   // 组装sql语句
    char sql[1024] = { 0 };
    // 好友表（friendid） + 用户表（user）的联合查询
    // 根据用户ID在好友表中查找用户的所有好友ID，并根据好友ID在用户表中查找好友的信息（ID、name、state）
    sprintf(sql, 
        "select a.id, a.name, a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d", 
        userid
    );

    std::vector<User> users;

    // MySQL mysql;
    // if(mysql.connect()) {
    //     MYSQL_RES* res = mysql.query(sql);
    //     if(res != nullptr) {
    //         MYSQL_ROW row;
    //         while((row = mysql_fetch_row(res)) != nullptr) {
    //             users.emplace_back(
    //                 atoi(row[0]),       // id
    //                 row[1],             // name
    //                 "",                 // pwd
    //                 row[2]              // state
    //             );
    //         }

    //         mysql_free_result(res);
    //     }
    // }

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
                    users.emplace_back(
                        atoi(row[0]),       // id
                        row[1],             // name
                        "",                 // pwd
                        row[2]              // state
                    );
                }

                mysql_free_result(res);
            }
        }
    }

    return users;
}