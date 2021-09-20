//
// Created by wonder on 2021/5/5.
//

#pragma once

#include <string>

#include "noncopyable.h"

// 日志级别  INFO,DEBUG,ERROR,FATAL
enum LogLevel{
    INFO,DEBUG,ERROR,FATAL
};

// 日志类（单例)
class Logger : noncopyable{
public:
    // 获取日志的单例对象
    static Logger & instance();
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);
private:
    int logLevel_;

    Logger(){}
};


#define LOG_INFO(logmsg,...) \
    do \
    {                              \
       Logger & logger = Logger::instance(); \
       logger.setLogLevel(INFO);   \
       char buf[1024] = {0};       \
       snprintf(buf,1024,logmsg,##__VA_ARGS__);                            \
       logger.log(buf);                       \
    }while(0)

#ifdef MUDEBUG
#define LOG_DEBUG(logmsg,...) \
    do \
    {                              \
       Logger & logger = Logger::instance(); \
       logger.setLogLevel(DEBUG);   \
       char buf[1024] = {0};       \
       snprintf(buf,1024,logmsg,##__VA_ARGS__);                            \
       logger.log(buf);                       \
    }while(0)
#else
#define LOG_DEBUG(logmsg,...)
#endif

#define LOG_ERROR(logmsg,...) \
    do \
    {                              \
       Logger & logger = Logger::instance(); \
       logger.setLogLevel(ERROR);   \
       char buf[1024] = {0};       \
       snprintf(buf,1024,logmsg,##__VA_ARGS__);                            \
       logger.log(buf);                       \
    }while(0)

#define LOG_FATAL(logmsg,...) \
    do \
    {                              \
       Logger & logger = Logger::instance(); \
       logger.setLogLevel(FATAL);   \
       char buf[1024] = {0};       \
       snprintf(buf,1024,logmsg,##__VA_ARGS__);                            \
       logger.log(buf);       \
       exit(-1);              \
    }while(0)

