# ClusterChatServer
## 基于Muduo、Redis、Nginx的集群聊天服务器
### 项目结构梳理
```
ClusterChatServer/
├── CMakeLists.txt                              # CMakeLists.txt构建文件
├── autobuild.sh                                # 构建脚本
├── include                                     
│   ├── public.hpp
│   ├── server
│   │   ├── ChatServer.hpp                      # 网络模块
│   │   ├── ChatService.hpp                     # 业务模块
│   │   ├── db  
│   │   │   └── MySql.hpp                       # 数据库基本操作模块（非连接池版）
│   │   ├── dbConnPool                          # 数据库基本操作模块（连接池版）
│   │   │   ├── DataBaseConn.hpp
│   │   │   └── DataBaseConnPool.hpp
│   │   ├── model                               
│   │   │   ├── Group.hpp                       # 对应allgroup表
│   │   │   ├── GroupModel.hpp                  # 对应allgroup表
│   │   │   ├── GroupUser.hpp                   # 对应groupuser表
│   │   │   ├── OfflineMessageModel.hpp         # 对应offlinemessage表
│   │   │   ├── User.hpp                        # 对应user表
│   │   │   ├── UserModel.hpp                   # 对应user表
│   │   │   └── friendModel.hpp                 # friend表
│   │   └── redis
│   │       └── redis.hpp                       # Redis的发布-订阅
│   └── tools
│       ├── Logger.hpp                          # 日志
│       └── Timestamp.hpp                       # 时间戳
├── src
│   ├── CMakeLists.txt
│   ├── client
│   │   ├── CMakeLists.txt
│   │   └── main.cpp
│   └── server
│       ├── CMakeLists.txt
│       ├── ChatServer.cpp
│       ├── ChatService.cpp
│       ├── db
│       │   └── MySql.cpp
│       ├── dbConnPool
│       │   ├── DataBaseConn.cpp
│       │   ├── DataBaseConnPool.cpp
│       │   └── config.json                     # 数据库连接池配置文件
│       ├── main.cpp
│       ├── model
│       │   ├── GroupModel.cpp
│       │   ├── OfflineMessageModel.cpp
│       │   ├── UserModel.cpp
│       │   └── friendModel.cpp
│       └── redis
│           └── redis.cpp
└── thirdparty
    └── json.hpp                                # json库
```
### 项目描述
&emsp;&emsp;基于`Muduo、Redis、Nginx、MySQL`构建聊天集群，解决跨服务器通信、数据库性能瓶颈问题。
### 开发环境
&emsp;&emsp;`Visual Studio Code + WSL2（Ubuntu-22.04）、CMake`。
### 项目内容
- 设计并实现基于`Muduo Reactor`模型的高并发网络核心模块，解耦网络`I/O`处理与业务逻辑；
- 采用`JSON`设计并实现高效的私有通信协议，用于客户端与服务器之间的消息传递；
- 配置`Nginx TCP`负载均衡，实现客户端请求在多个聊天服务节点上的动态分发；
- 利用`Redis`的发布/订阅功能，实现跨服务器的消息通信；
- 选用`MySQL`作为关系型数据库存储数据，完成`MySQL C API`的封装，实现对数据的增删改查；
- 设计并实现数据库连接池，通过复用建立的数据库连接，减少频繁建立和断开数据库连接带来的开销； 
- 使用`CMake`管理项目构建流程，通过`Shell`脚本自动化编译，提升开发效率。
### 数据库
```
mysql> show tables;
+----------------+
| Tables_in_chat |
+----------------+
| allgroup       |
| friend         |
| groupuser      |
| offlinemessage |
| user           |
+----------------+

mysql> desc allgroup;
+-----------+--------------+------+-----+---------+----------------+
| Field     | Type         | Null | Key | Default | Extra          |
+-----------+--------------+------+-----+---------+----------------+
| id        | int          | NO   | PRI | NULL    | auto_increment |
| groupname | varchar(50)  | NO   | UNI | NULL    |                |
| groupdesc | varchar(200) | YES  |     |         |                |
+-----------+--------------+------+-----+---------+----------------+

mysql> desc friend;
+----------+------+------+-----+---------+-------+
| Field    | Type | Null | Key | Default | Extra |
+----------+------+------+-----+---------+-------+
| userid   | int  | NO   | MUL | NULL    |       |
| friendid | int  | NO   |     | NULL    |       |
+----------+------+------+-----+---------+-------+

mysql> desc groupuser;
+-----------+--------------------------+------+-----+---------+-------+
| Field     | Type                     | Null | Key | Default | Extra |
+-----------+--------------------------+------+-----+---------+-------+
| groupid   | int                      | NO   | MUL | NULL    |       |
| userid    | int                      | NO   |     | NULL    |       |
| grouprole | enum('creator','normal') | YES  |     | NULL    |       |
+-----------+--------------------------+------+-----+---------+-------+

mysql> desc offlinemessage;
+---------+--------------+------+-----+---------+-------+
| Field   | Type         | Null | Key | Default | Extra |
+---------+--------------+------+-----+---------+-------+
| userid  | int          | NO   |     | NULL    |       |
| message | varchar(500) | NO   |     | NULL    |       |
+---------+--------------+------+-----+---------+-------+

mysql> desc user;
+----------+--------------------------+------+-----+---------+----------------+
| Field    | Type                     | Null | Key | Default | Extra          |
+----------+--------------------------+------+-----+---------+----------------+
| id       | int                      | NO   | PRI | NULL    | auto_increment |
| name     | varchar(50)              | YES  | UNI | NULL    |                |
| password | varchar(50)              | YES  |     | NULL    |                |
| state    | enum('online','offline') | YES  |     | offline |                |
+----------+--------------------------+------+-----+---------+----------------+
```
### `Nginx TCP`负载均衡配置
```
# nginx tcp loadbalance config
stream {
    upstream MyServer {
        server 127.0.0.1:6000 weight=1 max_fails=3 fail_timeout=30s;
        server 127.0.0.1:6002 weight=1 max_fails=3 fail_timeout=30s;
    }

    server {
        proxy_connect_timeout 1s;
        listen 8000;
        proxy_pass MyServer;
        tcp_nodelay on;
    }
}
```