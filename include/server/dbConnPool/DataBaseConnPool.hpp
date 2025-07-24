#ifndef __DBCONNPOOL_HPP__
#define __DBCONNPOOL_HPP__

#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <memory>
#include <condition_variable>

#include "DataBaseConn.hpp"

// 连接池类
// 线程安全的懒汉式单例模式
class DataBaseConnPool
{
public:
    // 删除拷贝构造函数和赋值运算符
    DataBaseConnPool(const DataBaseConnPool&) = delete;
    DataBaseConnPool& operator=(const DataBaseConnPool&) = delete;

    // 获取连接池实例
    static DataBaseConnPool* instance();

    // 获取数据库连接池中空闲的连接连接
    std::shared_ptr<DataBaseConn> getConnection();

    // 生产者线程函数
    void producerThreadFunc();

    // 回收空闲连接线程函数
    void cleanerThreadFunc();

private:
    // 构造函数私有化
    DataBaseConnPool();

    // 析构函数
    ~DataBaseConnPool();

    // 加载配置文件
    void loadConfigJsonFile(const std::string& configFilePath);

private:
    std::string ip_;                          // 数据库IP地址
    unsigned short port_;                     // 数据库端口
    std::string user_;                        // 数据库用户名
    std::string password_;                    // 数据库密码
    std::string dbname_;                      // 数据库名
    int initConnections_;                     // 初始连接数
    int maxConnections_;                      // 最大连接数
    int maxIdleTime_;                         // 最大空闲时间，单位s
    int maxWaitTime_;                         // 最大等待时间，单位ms

    std::queue<DataBaseConn*> connQue_;       // 连接队列
    std::mutex connQueMtx_;                   // 互斥锁
    std::atomic_int currentConnections_;      // 当前创建出的总连接数
    std::condition_variable connCondVar_;     // 条件变量，用于等待连接

    std::atomic_bool isPoolRunning_;          // 是否正在运行
};

#endif