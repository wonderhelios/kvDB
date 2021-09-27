//
// Created by wonder on 2021/9/26.
//

#include <sstream>
#include "DBServer.h"
#include "net/Logger.h"
#include "db/dbStatus.h"
#include "db/dbObj.h"

DBServer::DBServer(EventLoop *loop, const InetAddress &localAddr)
        : loop_(loop),
          server_(loop_, localAddr, "DBServer") {

    server_.setConnectionCallback(
            std::bind(&DBServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(
            std::bind(&DBServer::onMessage, this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));
    initDB();
}

void DBServer::initDB() {
    database_ = std::make_unique<Database>();

    // 绑定命令处理函数
    cmdDict.insert(std::make_pair("set",
                                  std::bind(&DBServer::setCommand, this, std::placeholders::_1)));
    cmdDict.insert(std::make_pair("get",
                                  std::bind(&DBServer::getCommand, this, std::placeholders::_1)));
    cmdDict.insert(std::make_pair("pexpire",
                                  std::bind(&DBServer::pExpiredCommand, this, std::placeholders::_1)));
    cmdDict.insert(std::make_pair("expire",
                                  std::bind(&DBServer::expiredCommand,this,std::placeholders::_1)));
}

void DBServer::onConnection(const TcpConnectionPtr &conn) {

}

void DBServer::onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp timestamp) {
    auto msg = buf->retrieveAsString();
    auto res = parseMsg(msg);

    conn->send(res);
}

void DBServer::start() {
    server_.start();
}

// 解析命令
std::string DBServer::parseMsg(const std::string &msg) {
    std::string res;
    std::istringstream ss(msg);
    std::string cmd, key, objKey, objValue;


    ss >> cmd;
    if(cmd.empty()){
        return dbStatus::notFound(" ").toString();
    }
    if (cmd == "set") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("Not Found Command").toString();
        } else {
            ss >> key;
            ss >> objKey;
            VctS vs = {cmd, key, objKey};
            res = it->second(std::move(vs));
        }
    } else if (cmd == "get") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("Not Found Command").toString();
        } else {
            ss >> key;
            VctS vs = {cmd, key};
            res = it->second(std::move(vs));
        }
    }else if(cmd == "pexpire"){
        auto it = cmdDict.find(cmd);
        if(it == cmdDict.end()){
            return dbStatus::notFound("Not Found Command").toString();
        }else{
            ss >> key;
            ss >> objKey;
            VctS vs = {cmd,key,objKey};
            res = it->second(std::move(vs));
        }
    }else if(cmd == "expire"){
        auto it = cmdDict.find(cmd);
        if(it == cmdDict.end()){
            return dbStatus::notFound("Not Found Command").toString();
        }else{
            ss >> key;
            ss >> objKey;
            VctS vs = {cmd,key,objKey};
            res = it->second(std::move(vs));
        }
    }else {
        return dbStatus::notFound("Not Found Command").toString();
    }
    return res;
}

std::string DBServer::setCommand(VctS &&argv) {
    if (argv.size() != 3) {
        return dbStatus::IOError("Parameter error").toString();
    }
    // 处理过期时间
    bool expired = database_->judgeKeyExpiredTime(dbObj::dbString,argv[1]);
    if(expired){
        database_->delKey(dbObj::dbString,argv[1]);
    }

    bool res = database_->addKey(dbObj::dbString, argv[1], argv[2], dbObj::defaultObjValue);

    return res ? dbStatus::Ok().toString() :
           dbStatus::IOError("set error").toString();
}

std::string DBServer::getCommand(VctS &&argv) {
    if (argv.size() != 2) {
        return dbStatus::IOError("Parameter error").toString();
    }
    // 处理过期时间
    bool expired = database_->judgeKeyExpiredTime(dbObj::dbString,argv[1]);
    if(expired){
        database_->delKey(dbObj::dbString,argv[1]);
        return dbStatus::IOError("Empty Content").toString();
    }

    std::string res = database_->getKey(dbObj::dbString, argv[1]);

    return res.empty() ? dbStatus::IOError("Empty Content").toString() :
           res;
}

std::string DBServer::pExpiredCommand(VctS &&argv) {
    if (argv.size() != 3) {
        return dbStatus::IOError("Parameter error").toString();
    }
    bool res = database_->setPExpireTime(dbObj::dbString, argv[1], atof(argv[2].c_str()));

    return res ? dbStatus::Ok().toString() :
           dbStatus::IOError("pExpire error").toString();
}

std::string DBServer::expiredCommand(VctS && argv) {
    if (argv.size() != 3) {
        return dbStatus::IOError("Parameter error").toString();
    }
    bool res = database_->setPExpireTime(dbObj::dbString, argv[1], atof(argv[2].c_str()) * Timestamp::kMicroSecondsPerMilliSecond);

    return res ? dbStatus::Ok().toString() :
           dbStatus::IOError("expire error").toString();
}