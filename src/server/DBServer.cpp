//
// Created by wonder on 2021/9/26.
//

#include <sstream>
#include <unistd.h>
#include <fstream>
#include "DBServer.h"
#include "net/Logger.h"
#include "db/dbStatus.h"
#include "db/dbObj.h"

DBServer::DBServer(EventLoop *loop, const InetAddress &localAddr)
        : loop_(loop),
          server_(loop_, localAddr, "DBServer"),
          lastSave_(Timestamp::invalid()) {

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
    for (int i = 0; i < DEFAULT_DB_NUM; ++i) {
        database_.emplace_back(std::make_unique<Database>());
    }
    dbIndex = 0;
    database_[dbIndex]->rdbLoad(dbIndex);
    // 绑定命令处理函数
    cmdDict.insert(std::make_pair("set",
                                  std::bind(&DBServer::setCommand, this, std::placeholders::_1)));
    cmdDict.insert(std::make_pair("get",
                                  std::bind(&DBServer::getCommand, this, std::placeholders::_1)));
    cmdDict.insert(std::make_pair("pexpire",
                                  std::bind(&DBServer::pExpiredCommand, this, std::placeholders::_1)));
    cmdDict.insert(std::make_pair("expire",
                                  std::bind(&DBServer::expiredCommand, this, std::placeholders::_1)));
    cmdDict.insert(std::make_pair("bgsave",
                                  std::bind(&DBServer::bgsaveCommand, this, std::placeholders::_1)));
    cmdDict.insert(std::make_pair("select",
                                  std::bind(&DBServer::selectCommand, this, std::placeholders::_1)));
    cmdDict.insert(std::make_pair("rpush",
                                  std::bind(&DBServer::rpushCommand, this, std::placeholders::_1)));
    cmdDict.insert(std::make_pair("rpop",
                                  std::bind(&DBServer::rpopCommand, this, std::placeholders::_1)));
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

void DBServer::rdbSave() {
    pid_t pid = fork();
    if (pid == 0) {
        LOG_INFO("child process");

        char buf[1024]{0};
        std::string path = getcwd(buf, 1024);
        assert(!path.empty());
        path += "/dump.rdb";

        std::ofstream out;
        out.open(path, std::ios::out | std::ios::app | std::ios::binary);
        if (!out.is_open()) {
            LOG_FATAL("RDB持久化失败...");
        }

        std::string str;
        // 存储RDB头
        str = saveHead();
        for (int i = 0; i < DEFAULT_DB_NUM; ++i) {
            if (database_[i]->getKeySize() == 0) {
                continue;
            }
            str += saveSelectDB(i);
            if (database_[i]->getKeyStringSize() != 0) {
                str += saveType(dbObj::dbString);
                auto obj = database_[i]->getKeyStringObj();
                for (auto it = obj.begin(); it != obj.end(); it++) {
                    str += saveExpiredTime(database_[i]->getKeyExpiredTime(dbObj::dbString, it->first));
                    str += saveKV(it->first, it->second);
                }
            }
            str.append("EOF");
            out.write(str.c_str(), str.size());
        }
        out.close();
        exit(0);
    } else if (pid > 0) {
        LOG_INFO("parent process");
    } else {
        LOG_ERROR("fork error");
    }
}

// 解析命令
std::string DBServer::parseMsg(const std::string &msg) {
    std::string res;
    std::istringstream ss(msg);
    std::string cmd, key, objKey, objValue;


    ss >> cmd;
    if (cmd.empty()) {
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
    } else if (cmd == "pexpire") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("Not Found Command").toString();
        } else {
            ss >> key;
            ss >> objKey;
            VctS vs = {cmd, key, objKey};
            res = it->second(std::move(vs));
        }
    } else if (cmd == "expire") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("Not Found Command").toString();
        } else {
            ss >> key;
            ss >> objKey;
            VctS vs = {cmd, key, objKey};
            res = it->second(std::move(vs));
        }
    } else if (cmd == "bgsave") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("Not Found Command").toString();
        } else {
            VctS vs = {cmd};
            res = it->second(std::move(vs));
        }
    } else if (cmd == "select") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("Not Found Command").toString();
        } else {
            ss >> key;
            VctS vs = {cmd, key};
            res = it->second(std::move(vs));
        }
    } else if(cmd == "rpush") {
        auto it = cmdDict.find(cmd);
        if(it == cmdDict.end()) {
            return dbStatus::notFound("Not Found Command").toString();
        }else {
            ss >> key;
            while(ss >> objKey){
                VctS vs = {cmd,key,objKey};
                res = it->second(std::move(vs));
            }
        }
    }else if(cmd == "rpop") {
        auto it = cmdDict.find(cmd);
        if(it == cmdDict.end()){
            return dbStatus::notFound("Not Found Command").toString();
        }else{
            ss >> key;
            VctS vs = {cmd,key};
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
    bool expired = database_[dbIndex]->judgeKeyExpiredTime(dbObj::dbString, argv[1]);
    if (expired) {
        database_[dbIndex]->delKey(dbObj::dbString, argv[1]);
    }

    bool res = database_[dbIndex]->addKey(dbObj::dbString, argv[1], argv[2], dbObj::defaultObjValue);

    return res ? dbStatus::Ok().toString() :
           dbStatus::IOError("set error").toString();
}

std::string DBServer::getCommand(VctS &&argv) {
    if (argv.size() != 2) {
        return dbStatus::IOError("Parameter error").toString();
    }
    // 处理过期时间
    bool expired = database_[dbIndex]->judgeKeyExpiredTime(dbObj::dbString, argv[1]);
    if (expired) {
        database_[dbIndex]->delKey(dbObj::dbString, argv[1]);
        return dbStatus::IOError("Empty Content").toString();
    }

    std::string res = database_[dbIndex]->getKey(dbObj::dbString, argv[1]);

    return res.empty() ? dbStatus::IOError("Empty Content").toString() :
           res;
}

std::string DBServer::pExpiredCommand(VctS &&argv) {
    if (argv.size() != 3) {
        return dbStatus::IOError("Parameter error").toString();
    }
    bool res = database_[dbIndex]->setPExpireTime(dbObj::dbString, argv[1], atof(argv[2].c_str()));

    return res ? dbStatus::Ok().toString() :
           dbStatus::IOError("pExpire error").toString();
}

std::string DBServer::expiredCommand(VctS &&argv) {
    if (argv.size() != 3) {
        return dbStatus::IOError("Parameter error").toString();
    }
    bool res = database_[dbIndex]->setPExpireTime(dbObj::dbString, argv[1],
                                                  atof(argv[2].c_str()) * Timestamp::kMicroSecondsPerMilliSecond);

    return res ? dbStatus::Ok().toString() :
           dbStatus::IOError("expire error").toString();
}

std::string DBServer::bgsaveCommand(VctS &&argv) {
    if (argv.size() != 1) {
        return dbStatus::IOError("Parameter error").toString();
    }
    bool res = checkSaveCondition();
    return res ? dbStatus::Ok().toString() :
           dbStatus::IOError("bgsave error").toString();
}

std::string DBServer::selectCommand(VctS &&argv) {
    if (argv.size() != 2) {
        return dbStatus::IOError("Parameter error").toString();
    }
    int idx = atoi(argv[1].c_str());
    dbIndex = idx - 1;
    database_[dbIndex]->rdbLoad(dbIndex);   // 加载rdb文件
    return dbStatus::Ok().toString();
}

std::string DBServer::rpushCommand(VctS &&argv) {
    if (argv.size() < 3) {
        return dbStatus::IOError("Parameter error").toString();
    }
    int flag;
    for (int i = 2; i < argv.size(); i++) {
        flag = database_[dbIndex]->addKey(dbObj::dbList, argv[1], argv[i], dbObj::defaultObjValue);
    }
    if (flag) {
        return dbStatus::Ok().toString();
    } else {
        return dbStatus::IOError("rpush error").toString();
    }
}

std::string DBServer::rpopCommand(VctS &&argv) {
    if (argv.size() != 2) {
        return dbStatus::IOError("Parameter error").toString();
    }
    std::string res = database_[dbIndex]->rpopList(argv[1]);
    if (res.empty()) {
        return dbStatus::IOError("rpop error").toString();
    } else {
        return '+' + res;
    }
}

std::string DBServer::saveHead() {
    std::string tmp = "REDIS0006";
    return tmp;
}

std::string DBServer::saveSelectDB(const int index) {
    return "SD" + std::to_string(index);
}

std::string DBServer::saveExpiredTime(const Timestamp expiredTime) {
    return "ST" + std::to_string(expiredTime.microSecondsSinceEpoch());
}

std::string DBServer::saveType(const int type) {
    return "^" + std::to_string(type);
}

std::string DBServer::saveKV(const std::string &key, const std::string &value) {
    char buf[1024];
    sprintf(buf, "!%d#%s!%d$%s", static_cast<int>(key.size()),
            key.c_str(),
            static_cast<int>(value.size()),
            value.c_str());
    return std::string(buf);
}

bool DBServer::checkSaveCondition() {
    Timestamp save_interval = Timestamp::now() - lastSave_;
    if (save_interval > dbObj::rdbDefaultTime) {
        LOG_INFO("bgsaving...");
        rdbSave();
        lastSave_ = Timestamp::now();
        return true;
    }
    return false;
}