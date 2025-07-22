#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <functional>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "Group.hpp"
#include "User.hpp"
#include "public.hpp"
#include "json.hpp"

using json = nlohmann::json;

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
std::vector<User> g_currentUserFriendList;
// 记录当前登录用户群组列表信息
std::vector<Group> g_currentUserGroupList;
// 
bool g_isMainMenuRunning = false;
// 用于主、子线程之间通信的信号量
sem_t rwsem;
// 记录登录状态
std::atomic_bool g_isLoginSuccess;

// 显示当前登录成功用户的基本信息
void showCurrentUserData();
// 接收线程
void readTaskHandler(int clientfd);
// 主聊天页面
void mainMenu(int);
// 获取系统时间
std::string getCurrentTime();

// 聊天客户端，主线程作为发送线程，子线程作为接收线程
int main(int argc, char** argv)
{
    if(argc < 3) {
        std::cerr << "command invalid! example: ./client 127.0.0.1 6000\n";
        exit(-1);
    }

    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建客户端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd == -1) {
        std::cerr << "socket create error\n";
        exit(-1);
    }

    // 
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    //
    if(connect(clientfd, (const struct sockaddr *)&server, sizeof(sockaddr_in)) == -1) {
        std::cerr << "connect server error\n";
        close(clientfd);
        exit(-1);
    }

    // 初始化信号量
    sem_init(&rwsem, 0, 0);

    // 连接服务器，启动用于接收服务器消息的子线程
    std::thread readTask(readTaskHandler, clientfd);
    readTask.detach();

    // main线程用于接收用户输入，负责发送数据
    for (;;)
    {
        // 显示首页面菜单 登录、注册、退出
        std::cout << "========================" << std::endl;
        std::cout << "1.login" << std::endl;
        std::cout << "2.register" << std::endl;
        std::cout << "3.quit" << std::endl;
        std::cout << "========================" << std::endl;
        std::cout << "choice: ";

        int choice = 0;
        std::cin >> choice;
        std::cin.get();         // 读掉缓冲区残留的回车

        switch(choice) {
            case 1: {   // 登录业务
                int id = 0;
                char pwd[50] = { 0 };

                // 输入用户ID
                std::cout << "userid: ";
                std::cin >> id;
                std::cin.get();     // 读掉缓冲区残留的回车

                // 输入密码
                std::cout << "userpassword: ";
                std::cin.getline(pwd, 50);

                json js;
                js["msgid"] = MSG_LOGIN;
                js["id"] = id;
                js["password"] = pwd;
                std::string request = js.dump();    // 序列化

                g_isLoginSuccess = false;

                // 向服务器发送json序列化结果
                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if(len == -1) {
                    std::cerr << "send login msg error:" << request << std::endl;
                } 

                // 阻塞等待子线程处理完成登录业务
                sem_wait(&rwsem);
                
                if(g_isLoginSuccess) {
                    // 进入聊天主页面
                    g_isMainMenuRunning = true;
                    mainMenu(clientfd);
                }

                break;
            }
            case 2: {   // 注册业务
                char name[50] = { 0 };
                char pwd[50] = { 0 };

                // 输入用户名
                std::cout << "username: ";
                std::cin.getline(name, 50);

                // 输入用户密码
                std::cout << "userpassword: ";
                std::cin.getline(pwd, 50);

                json js;
                js["msgid"] = MSG_REG;
                js["name"] = name;
                js["password"] = pwd;
                std::string request = js.dump();    // 序列化

                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if(len == -1) {
                    std::cerr << "send reg msg error: " << request << "\n";
                }

                // 阻塞等待子线程处理完成注册业务
                sem_wait(&rwsem);

                break;
            }
            case 3: {
                // 退出业务
                close(clientfd);
                // 销毁信号量
                sem_destroy(&rwsem);
                // 
                exit(0);
            }
            default: {
                // 默认
                std::cerr << "invalid input!" << std::endl;
                break;
            }
        }
    }

    return 0;
}

// 处理登录业务
void doLoginResponse(json& responsejs) {
    // 反序列化
    if(responsejs["errno"].get<int>() != 0) {
        // 登录失败
        std::cerr << responsejs["errmsg"] << "\n";

        g_isLoginSuccess = false;
    }
    else {
        // 登录成功
        // 记录当前用户的ID和name
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);

        // 记录当前用户的好友列表
        if(responsejs.contains("friends")) {
            g_currentUserFriendList.clear();

            std::vector<std::string> strVec = responsejs["friends"];
            for(auto&& str : strVec) {
                json js = json::parse(str);
                g_currentUserFriendList.emplace_back(
                    js["id"].get<int>(),
                    js["name"],
                    "",
                    js["state"]
                );
            }
        }

        // 记录当前用户的群组列表信息
        if(responsejs.contains("groups")) {
            g_currentUserGroupList.clear();

            std::vector<std::string> vec1 = responsejs["groups"];
            for(auto&& groupStr : vec1) {
                json grpjs = json::parse(groupStr);
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                std::vector<std::string> vec2 = grpjs["users"];
                for(auto&& userStr : vec2) {
                    json js = json::parse(userStr);
                    GroupUser user;
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);

                    group.getUsers().push_back(user);
                }

                g_currentUserGroupList.push_back(group);
            }
        }

        // 显示用户基本信息
        showCurrentUserData();

        // 显示当前用户的离线消息（个人聊天信息/群组信息）
        if(responsejs.contains("offlinemsg")) {
            std::vector<std::string> vec = responsejs["offlinemsg"];
            for(auto&& str : vec) {
                json js = json::parse(str);

                if(js["msgid"].get<int>() == MSG_ONE_CHAT) {
                    std::cout << js["time"].get<std::string>() << " [" 
                                << js["id"] << "] "
                                << js["name"].get<std::string>() << " said: " 
                                << js["msg"].get<std::string>() << "\n";
                }
                else {
                    std::cout << "群消息[" << js["groupid"] << "]: "
                                << js["time"].get<std::string>() << " [" 
                                << js["id"] << "] "
                                << js["name"].get<std::string>() << " said: " 
                                << js["msg"].get<std::string>() << "\n";
                }
            }
        }

        g_isLoginSuccess = true;
    }  
}

// 处理注册业务
void doRegResponse(json& responsejs) {
    if(responsejs["errno"].get<int>() != 0) {
        // 注册失败
        std::cerr << "current user is already exist, register error!\n";
    }
    else {
        // 注册成功
        std::cout << "current user register success, userid: " << responsejs["id"] << "\n";
    }
}

// 接收线程
void readTaskHandler(int clientfd)
{
    for(;;) {
        char buffer[1024] = { 0 };
        int len = recv(clientfd, buffer, 1024, 0);
        if(len == 0 || len == 1) {
            close(clientfd);
            exit(-1);
        }

        json js = json::parse(buffer);
        int msgType = js["msgid"].get<int>();

        if(msgType == MSG_LOGIN_ACK) {      // 处理登录业务
            doLoginResponse(js);
            // 通知主线程登录业务处理完成
            sem_post(&rwsem);

            continue;
        }

        if(msgType == MSG_REG_ACK) {        // 处理注册业务
            doRegResponse(js);
            // 通知主线程注册业务处理完成
            sem_post(&rwsem);

            continue;
        }

        if(msgType == MSG_ONE_CHAT) {       // 处理点对点聊天业务
            std::cout << js["time"].get<std::string>() 
                        << " [" << js["id"] << "] "
                        << js["name"].get<std::string>() << " said: " 
                        << js["msg"].get<std::string>() << "\n";

            continue;
        }

        if(msgType == MSG_GROUP_CHAT) {     // 处理群聊天业务
            std::cout << "群消息[" << js["groupid"] 
                        << "]: " << js["time"].get<std::string>() << " [" 
                        << js["id"] << "] "
                        << js["name"].get<std::string>() << " said: " 
                        << js["msg"].get<std::string>() << "\n";

            continue;
        }
    }
}

///////////////////////////////////// 表驱动 /////////////////////////////////////
// 
void help(int fd = 0, const std::string& = "");
// 
void chat(int, const std::string&);
// 
void addfriend(int, const std::string&);
// 
void creategroup(int, const std::string&);
// 
void addgroup(int, const std::string&);
// 
void groupchat(int, const std::string&);
// 
void loginout(int, const std::string&);

// 系统支持的客户端命令列表
std::unordered_map<std::string, std::string> commandMap = {
    { "help", "显示所有支持的命令, 格式help" },
    { "chat", "一对一聊天, 格式chat:friendid:message" },
    { "addfriend", "添加好友, 格式addfriend:friendid" },
    { "creategroup", "创建群组, 格式creategroup:groupname:groupdesc" },
    { "addgroup", "加入群组, 格式addgroup:groupid" },
    { "groupchat", "群聊, 格式groupchat:groupid:message" },
    { "loginout", "注销, 格式loginout" }
};

// 注册系统支持的客户端命令行
std::unordered_map<std::string, std::function<void(int, const std::string&)>> commandHandlerMap = {
    { "help", help },
    { "chat", chat },
    { "addfriend", addfriend },
    { "creategroup", creategroup },
    { "addgroup", addgroup },
    { "groupchat", groupchat },
    { "loginout", loginout }
};

// 主聊天页面
void mainMenu(int clientfd)
{
    help();

    char buffer[1024] = { 0 };
    while(g_isMainMenuRunning) {
        // 获取输入
        std::cin.getline(buffer, 1024);

        std::string commandbuf(buffer);
        std::string command;                // 存储命令

        int idx = commandbuf.find(":");
        if(idx == -1) {
            command = commandbuf;
        }
        else {
            command = commandbuf.substr(0, idx);
        }

        auto it = commandHandlerMap.find(command);
        if(it == commandHandlerMap.end()) {
            std::cerr << "invalid input command!\n";
            continue;
        }

        // 调用相应命令的事件处理回调
        // 对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
    }
}

//
void help(int, const std::string&) {
    std::cout << "show command list >>> \n";
    for(auto&& p : commandMap) {
        std::cout << p.first << " : " << p.second << "\n";
    }
    std::cout << "\n";
}

// 
void addfriend(int clientfd, const std::string& str) {
    int friendid = atoi(str.c_str());

    json js;
    js["msgid"] = MSG_ADD_FRIEND;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    
    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1) {
        std::cerr << "send addfriend msg error -> " << buffer << "\n";
    }
}

// 
void chat(int clientfd, const std::string& str) {
    int idx = str.find(":");
    if(idx == -1) {
        std::cout << "chat command invalid\n";

        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    std::string msg = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = MSG_ONE_CHAT;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = msg;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1) {
        std::cerr << "send chat msg error -> " << buffer << "\n";
    }
}

// 
void creategroup(int clientfd, const std::string& str) {
    int idx = str.find(":");
    if(idx == -1) {
        std::cout << "creategroup command invalid\n";

        return;
    }

    std::string groupname = str.substr(0, idx);
    std::string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = MSG_CREATE_GROUP;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;

    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1) {
        std::cerr << "send creategroup msg error -> " << buffer << "\n";
    }
}

// 
void addgroup(int clientfd, const std::string& str) {
    int groudid = atoi(str.c_str());

    json js;
    js["msgid"] = MSG_ADD_GROUP;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groudid;
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1) {
        std::cerr << "send addgroup msg error -> " << buffer << "\n";
    }
}

// 
void groupchat(int clientfd, const std::string& str) {
    int idx = str.find(":");
    if(idx == -1) {
        std::cerr << "groupchat command invalid!\n";

        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = MSG_GROUP_CHAT;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1) {
        std::cerr << "send groupchat msg error -> " << buffer << "\n";
    }
}

// 
void loginout(int clientfd, const std::string& str) {
    json js;
    js["msgid"] = MSG_LOGINOUT;
    js["id"] = g_currentUser.getId();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1) {
        std::cerr << "send loginout msg error -> " << buffer << "\n";
    }
    else {
        g_isMainMenuRunning = false;
    }
}

// 显示当前登录成功用户的基本信息
void showCurrentUserData() {
    std::cout << "======================login user======================\n";
    std::cout << "current login user >> id: " 
              << g_currentUser.getId() << " name: " 
              << g_currentUser.getName() << "\n";
    
    if(!g_currentUserFriendList.empty()) {
        for(auto&& user : g_currentUserFriendList) {
            std::cout << "friends >> id: " 
                      << user.getId() << " name: " 
                      << user.getName() << " " 
                      << user.getState() << "\n"; 
        }
    }

    std::cout << "---------------------group list-----------------------\n";
    if(!g_currentUserGroupList.empty()) {
        for(auto&& group : g_currentUserGroupList) {
            std::cout << group.getId() << " " 
                      << group.getName() << " " 
                      << group.getDesc() << "\n";

            for(auto&& user : group.getUsers()) {
                std::cout << "  " 
                          << user.getId() << " " 
                          << user.getName() << " " 
                          << user.getState() << " "
                          << user.getRole() << "\n";
            }
        }
    }

    std::cout << "======================================================\n";
}


// 获取系统时间
std::string getCurrentTime() {
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    struct tm *ptm = localtime(&tt);
    char date[60] = {0};

    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, 
            (int)ptm->tm_mon + 1, 
            (int)ptm->tm_mday,
            (int)ptm->tm_hour, 
            (int)ptm->tm_min, 
            (int)ptm->tm_sec
    );

    return std::string(date);
}