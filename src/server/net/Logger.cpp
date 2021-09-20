//
// Created by wonder on 2021/5/5.
//
#include <iostream>

#include "Logger.h"
#include "Timestamp.h"

// 获取日志的单例对象
Logger & Logger::instance() {
    static Logger logger;
    return logger;
}
// 设置日志级别
void Logger::setLogLevel(int level) {
    logLevel_ = level;
}
// 写日志 [级别] time:msg
void Logger::log(std::string msg) {
    switch (logLevel_) {
        case INFO:
            std::cout<<"[INFO]";
            break;
        case DEBUG:
            std::cout<<"[DEBUG]";
            break;
        case ERROR:
            std::cout<<"[ERROR]";
            break;
        case FATAL:
            std::cout<<"[FATAL]";
            break;
        default:
            break;
    }
    // 输出time和msg
    std::cout<< Timestamp::now().toString() << " : " << msg << std::endl;
}