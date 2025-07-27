#ifndef __TIMESTAMP_HPP__
#define __TIMESTAMP_HPP__

#include <iostream>
#include <string>
#include <time.h>

class Timestamp
{
public:
    // 默认构造函数
    Timestamp()
        : microSecondsSinceEpoch_(0)
    {}

    // 构造函数
    explicit Timestamp(int64_t microSecondsSinceEpoch)
        : microSecondsSinceEpoch_(microSecondsSinceEpoch)
    {}

    // 析构函数
    ~Timestamp() = default;

    // 获取当前时间
    static Timestamp now() {
        return Timestamp(time(NULL));
    }

    // 转为字符串
    std::string toString() const {
        char buff[64] = { 0 };
        struct tm* time = localtime(&microSecondsSinceEpoch_);
        snprintf(buff, sizeof(buff), "%4d/%02d/%02d %02d:%02d:%02d", 
            time->tm_year + 1900, 
            time->tm_mon + 1, 
            time->tm_mday,
            time->tm_hour, 
            time->tm_min, 
            time->tm_sec
        );

        return buff;
    }

private:
    int64_t microSecondsSinceEpoch_;
};

#endif