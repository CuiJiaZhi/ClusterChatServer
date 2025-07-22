#include "GroupModel.hpp"
#include "MySql.hpp"

// 创建群组
// 向数据库的allgroup表中增加群组信息
bool GroupModel::createGroup(Group& group) {
    // 组装sql语句
    char sql[1024] = { 0 };
    sprintf(sql, 
        "insert into allgroup(groupname, groupdesc) values('%s', '%s')", 
        group.getName().c_str(), group.getDesc().c_str()
    );

    MySQL mysql;
    if(mysql.connect()) {
        if(mysql.update(sql)) {
            group.setId(mysql_insert_id(mysql.getConnection()));

            return true;
        }
    }

    return false;
}

// 加入群组
void GroupModel::addGroup(int userid, int groupid, const std::string& role) {
    // 组装sql语句
    char sql[1024] = { 0 };
    sprintf(sql, 
        "insert into groupuser values('%d', '%d', '%s')", 
        groupid, userid, role.c_str()
    );

    MySQL mysql;
    if(mysql.connect()) {
        mysql.update(sql);
    }
}

// 查询用户所在的所有群组信息
std::vector<Group> GroupModel::queryGroups(int userid) {
    /*
        - 根据userid在groupuser表中查询当前用户所属的所有群组信息
        - 根据当前用户所属的所有群组信息，查询所有群组下所有用户的ID，
          并根据user表进行多表联合查询，获取所有群组下所有用户的详细信息
    */
    // 组装sql语句
    char sql[1024] = { 0 };
    sprintf(sql, 
        "select a.id, a.groupname, a.groupdesc from allgroup a inner join \
        groupuser b on a.id = b.groupid where b.userid = %d", 
        userid
    );

    std::vector<Group> groupVec;

    MySQL mysql;
    if(mysql.connect()) {
        // 根据userid在groupuser表中查询当前用户所属的所有群组信息
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr) {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr) {
                groupVec.emplace_back(
                        atoi(row[0]),   // id
                        row[1],         // name
                        row[2]          // desc
                );
            }

            mysql_free_result(res);
        }
    }

    /*
        根据当前用户所属的所有群组信息，查询所有群组下所有用户的ID，
        并根据user表进行多表联合查询，获取所有群组下所有用户的详细信息
    */
    for(auto&& group : groupVec) {
        sprintf(sql, 
            "select a.id, a.name, a.state, b.grouprole from user a inner join \
            groupuser b on b.userid = a.id where b.groupid = %d", 
            group.getId()
        );

        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr) {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr) {
                group.getUsers().emplace_back(
                    atoi(row[0]),   // id
                    row[1],         // name
                    "",             // pwd
                    row[2],         // state
                    row[3]          // role
                );
            }

            mysql_free_result(res);
        }
    }

    return groupVec;
}

// 根据指定的groupid查询群组用户id列表（除userid自身）
std::vector<int> GroupModel::queryGroupUsers(int userid, int groupid) {
    char sql[1024] = { 0 };
    sprintf(sql, 
        "select userid from groupuser where groupid = %d and userid != %d", 
        groupid, userid
    );

    std::vector<int> idVec;

    MySQL mysql;
    if(mysql.connect()) {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr) {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr) {
                idVec.push_back(atoi(row[0]));
            }

            mysql_free_result(res);
        }
    }

    return idVec;
}