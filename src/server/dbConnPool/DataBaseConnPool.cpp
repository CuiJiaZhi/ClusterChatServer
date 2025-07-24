#include "DataBaseConnPool.hpp"
#include "public.hpp"
#include "json.hpp"
#include <fstream>
#include <stdexcept>

// 获取连接池实例
DataBaseConnPool* DataBaseConnPool::instance() {
    // C++11保证静态局部变量的初始化是线程安全的
    static DataBaseConnPool pool;   // 懒汉式单例

    return &pool;
}

// 消费者，获取数据库连接池中空闲的连接连接
std::shared_ptr<DataBaseConn> DataBaseConnPool::getConnection() {
    std::unique_lock<std::mutex> lock(connQueMtx_);
    while(connQue_.empty()) {   
        // 如果连接队列为空，等待一段时间
        if(connCondVar_.wait_for(lock, std::chrono::milliseconds(maxWaitTime_))
            == std::cv_status::timeout)
        {
            if(!isPoolRunning_) {
                // 如果连接池不再运行，返回空指针
                return nullptr; 
            }

            if(connQue_.empty()) {
                // 如果连接池仍在运行，但连接队列为空，且等待时间已到，返回空指针
                return nullptr; 
            }
        }
    }

    // 自定义智能指针的析构函数，使得智能指针析构时，不将连接对象删除，而是将连接对象放回连接池
    auto deleter = [&](DataBaseConn* conn) {
        if(isPoolRunning_) {
            // 如果连接池仍在运行，更新连接的存活时间
            conn->updateAliveTime(); 
            // 加锁
            std::unique_lock<std::mutex> lock(connQueMtx_);
            // 将连接对象放回连接池
            connQue_.push(conn);
        }
        else {
            delete conn; // 如果连接池不再运行，删除连接对象
        }
    };

    std::shared_ptr<DataBaseConn> ptr(connQue_.front(), deleter);

    connQue_.pop();

    // 消费完连接后，通知生产者线程检查一下，如果队列为空，则创建新连接
    connCondVar_.notify_all();

    return ptr;
}

// 生产者线程函数
void DataBaseConnPool::producerThreadFunc() {
    while(isPoolRunning_) {
        // 加锁
        std::unique_lock<std::mutex> lock(connQueMtx_);

        // 当连接队列为空时，connQue_.empty()为true/线程池不再运行，生产者停止阻塞
        connCondVar_.wait(lock, [this]() {
            return !isPoolRunning_ || connQue_.empty();
        });

        if(!isPoolRunning_) {
            return; // 如果连接池不再运行，退出线程
        }

        if(currentConnections_ < maxConnections_) {
            // 创建新的连接
            DataBaseConn* conn = new DataBaseConn();
            conn->connect(ip_, port_, user_, password_, dbname_);
            conn->updateAliveTime(); // 更新连接的存活时间
            connQue_.push(conn);
            currentConnections_++;
        }   

        // 通知消费者线程
        connCondVar_.notify_all();
    }
}

// 回收空闲连接线程函数
void DataBaseConnPool::cleanerThreadFunc() {
    while(isPoolRunning_) {
        // 模拟定时效果
        std::this_thread::sleep_for(std::chrono::seconds(maxIdleTime_));
        if(!isPoolRunning_) {
            return;     // 如果连接池不再运行，退出线程
        }
        // 扫描队列，回收满足条件的连接
        // 加锁
        std::unique_lock<std::mutex> lock(connQueMtx_);
        // 如果当前连接数大于初始连接数
        while(currentConnections_ > initConnections_) {
            DataBaseConn* conn = connQue_.front();
            if(conn->getAliveTime() >= maxIdleTime_) {
                // 如果连接的存活时间超过最大空闲时间，删除连接
                connQue_.pop();                 // 从队列中移除
                currentConnections_--;
                delete conn;                    // 删除连接对象
            } 
            else {
                // 如果队头的连接的存活时间未超过最大空闲时间，退出循环
                break;
            }
        }
    }
}

// 构造函数私有化
DataBaseConnPool::DataBaseConnPool() 
    : ip_("")
    , port_(0)
    , user_("")
    , password_("")
    , dbname_("")
    , initConnections_(0)
    , maxConnections_(0)
    , maxIdleTime_(0)
    , maxWaitTime_(0)
    , currentConnections_(0)
    , isPoolRunning_(true)
{
    // 加载配置项目
    loadConfigJsonFile("src/server/dbConnPool/config.json");

    // 检查配置项的有效性
    if(initConnections_ <= 0 || maxConnections_ <= 0 ||
       maxIdleTime_     <= 0 || maxWaitTime_    <= 0 ||
       initConnections_ > maxConnections_) 
    {
        throw std::invalid_argument("Invalid connection pool configuration");
    }

    // 初始化连接池
    for(int i = 0; i < initConnections_; ++i) {
        DataBaseConn* conn = new DataBaseConn();                // 创建新的连接对象
        conn->connect(ip_, port_, user_, password_, dbname_);   // 连接数据库
        conn->updateAliveTime();                                // 更新连接的存活时间
        connQue_.push(conn);                                    // 将连接对象放入连接池
        currentConnections_++;                                  // 更新当前连接数
    }

    // 启动新线程，作为生产者线程
    std::thread producer(
        std::bind(&DataBaseConnPool::producerThreadFunc, this)
    );
    // 分离线程，避免阻塞主线程
    // 这样可以让生产者线程在后台运行
    // 生产者线程会不断地检查连接池，如果连接池为空，则创建新的连接
    // 如果连接池不为空，则等待消费者线程使用连接
    producer.detach();

    // 启动新线程（定时线程），用于回收超过最大空闲时间的连接
    std::thread cleaner(
        std::bind(&DataBaseConnPool::cleanerThreadFunc, this)
    );
    // 分离线程，避免阻塞主线程
    // 这样可以让清理线程在后台运行
    // 清理线程会定时检查连接池中的连接，如果连接的存活时间超过最大空闲时间，则删除连接
    // 如果连接的存活时间未超过最大空闲时间，则退出循环
    cleaner.detach();
}

// 析构函数
DataBaseConnPool::~DataBaseConnPool() {
    // 停止连接池运行
    isPoolRunning_ = false;

    // 唤醒所有阻塞的线程
    connCondVar_.notify_all();

    // 清理连接队列
    while(!connQue_.empty()) {
        delete connQue_.front();
        connQue_.pop();
    }
}

// 加载配置文件
void DataBaseConnPool::loadConfigJsonFile(const std::string& configFilePath) {
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Could not open config file: " + configFilePath);
    }

    nlohmann::json configJson;
    configFile >> configJson;

    // 数据库IP地址
    ip_ = configJson["ip"].get<std::string>();
    // 数据库端口
    port_ = configJson["port"].get<unsigned short>();
    // 数据库用户名
    user_ = configJson["user"].get<std::string>();
    // 数据库密码
    password_ = configJson["password"].get<std::string>();
    // 数据库名
    dbname_ = configJson["dbname"].get<std::string>();
    // 初始连接数
    initConnections_ = configJson["initConnections"].get<int>();
    // 最大连接数
    maxConnections_ = configJson["maxConnections"].get<int>();
    // 最大空闲时间
    maxIdleTime_ = configJson["maxIdleTime"].get<int>();
    // 最大等待时间
    maxWaitTime_ = configJson["maxWaitTime"].get<int>();
}

