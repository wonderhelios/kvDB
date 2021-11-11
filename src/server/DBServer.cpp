//
// Created by wonder on 2021/9/26.
//

#include <sstream>
#include <unistd.h>
#include <fstream>
#include <cfloat>
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
    cmdDict.insert(std::make_pair("hset",
                                  std::bind(&DBServer::hsetCommand, this, std::placeholders::_1)));
    cmdDict.insert(std::make_pair("hget",
                                  std::bind(&DBServer::hgetCommand, this, std::placeholders::_1)));
    cmdDict.insert(std::make_pair("hgetall",
                                  std::bind(&DBServer::hgetAllCommand, this, std::placeholders::_1)));
    cmdDict.insert(std::make_pair("sadd",
                                  std::bind(&DBServer::saddCommand, this, std::placeholders::_1)));
    cmdDict.insert(std::make_pair("smembers",
                                  std::bind(&DBServer::smembersCommand, this, std::placeholders::_1)));
    cmdDict.insert(std::make_pair("zadd",
                                  std::bind(&DBServer::zaddCommand,this,std::placeholders::_1)));
    cmdDict.insert(std::make_pair("zcard",
                                  std::bind(&DBServer::zcardCommand,this,std::placeholders::_1)));
    cmdDict.insert(std::make_pair("zrange",
                                  std::bind(&DBServer::zrangeCommand,this,std::placeholders::_1)));
    cmdDict.insert(std::make_pair("zgetall",
                                  std::bind(&DBServer::zgetAllCommand,this,std::placeholders::_1)));

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
        out.open(path, std::ios::out | std::ios::trunc | std::ios::binary);
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
            // String
            if (database_[i]->getKeyStringSize() != 0) {
                str += saveType(dbObj::dbString);
                auto obj = database_[i]->getKeyStringObj();
                for (auto it = obj.begin(); it != obj.end(); it++) {
                    str += saveExpiredTime(database_[i]->getKeyExpiredTime(dbObj::dbString, it->first));
                    str += saveKV(it->first, it->second);
                }
            }
            // List
            if (database_[i]->getKeyListSize() != 0) {
                str += saveType(dbObj::dbList);
                auto it = database_[i]->getKeyListObj().begin();
                for (; it != database_[i]->getKeyListObj().end(); it++) {
                    str += saveExpiredTime(database_[i]->getKeyExpiredTime(dbObj::dbList, it->first));
                    auto iter = it->second.begin();
                    std::string tmp = '!' + std::to_string(it->second.size());
                    for (; iter != it->second.end(); iter++) {
                        tmp += '!' + std::to_string(iter->size()) + '$' + iter->c_str();
                    }
                    str += '!' + std::to_string(it->first.size()) + '#' + it->first.c_str() + tmp;
                }
            }
            // Hash
            if (database_[i]->getKeyHashSize() != 0) {
                str += saveType(dbObj::dbHash);
                auto it = database_[i]->getKeyHashObj().begin();
                for (; it != database_[i]->getKeyHashObj().end(); it++) {
                    str += saveExpiredTime(database_[i]->getKeyExpiredTime(dbObj::dbHash, it->first));
                    auto iter = it->second.begin();
                    std::string tmp = '!' + std::to_string(it->second.size());
                    for (; iter != it->second.end(); iter++) {
                        tmp += saveKV(iter->first, iter->second);
                    }
                    str += '!' + std::to_string(it->first.size()) + '#' + it->first.c_str() + tmp;
                }
            }
            // Set
            if (database_[i]->getKeySetSize() != 0) {
                str += saveType(dbObj::dbSet);
                auto it = database_[i]->getKeySetObj().begin();
                for (; it != database_[i]->getKeySetObj().end(); it++) {
                    str += saveExpiredTime(database_[i]->getKeyExpiredTime(dbObj::dbSet, it->first));
                    auto iter = it->second.begin();
                    std::string tmp = '!' + std::to_string(it->second.size());
                    for (; iter != it->second.end(); iter++) {
                        tmp += '!' + std::to_string(iter->size()) + '$' + iter->c_str();
                    }
                    str += '!' + std::to_string(it->first.size()) + '#' + it->first.c_str() + tmp;
                }
            }
            // ZSet
            if(database_[i]->getKeyZSetSize() != 0){
                str += saveType(dbObj::dbZSet);
                auto it = database_[i]->getKeyZSetObj().begin();
                for(;it != database_[i]->getKeyZSetObj().end();it++){
                    str += saveExpiredTime(database_[i]->getKeyExpiredTime(dbObj::dbZSet,it->first));
                    std::string tmp = '!' + std::to_string(it->second->getLength());
                    rangespec spec(DBL_MIN,DBL_MAX);
                    std::vector<skiplistNode*> vecSkip(it->second->getNodeInRange(spec));
                    for(int j=0;j < vecSkip.size();j++){
                        tmp+= saveKV(vecSkip[j]->obj_, std::to_string(vecSkip[j]->score_));
                    }
                    str += '!' + std::to_string(it->first.size()) + '#' + it->first.c_str() + tmp;
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
            return dbStatus::notFound("command").toString();
        } else {
            ss >> key;
            ss >> objKey;
            VctS vs = {cmd, key, objKey};
            res = it->second(std::move(vs));
        }
    } else if (cmd == "get") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("command").toString();
        } else {
            ss >> key;
            VctS vs = {cmd, key};
            res = it->second(std::move(vs));
        }
    } else if (cmd == "pexpire") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("command").toString();
        } else {
            ss >> key;
            ss >> objKey;
            VctS vs = {cmd, key, objKey};
            res = it->second(std::move(vs));
        }
    } else if (cmd == "expire") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("command").toString();
        } else {
            ss >> key;
            ss >> objKey;
            VctS vs = {cmd, key, objKey};
            res = it->second(std::move(vs));
        }
    } else if (cmd == "bgsave") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("command").toString();
        } else {
            VctS vs = {cmd};
            res = it->second(std::move(vs));
        }
    } else if (cmd == "select") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("command").toString();
        } else {
            ss >> key;
            VctS vs = {cmd, key};
            res = it->second(std::move(vs));
        }
    } else if (cmd == "rpush") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("command").toString();
        } else {
            ss >> key;
            while (ss >> objKey) {
                VctS vs = {cmd, key, objKey};
                res = it->second(std::move(vs));
            }
        }
    } else if (cmd == "rpop") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("command").toString();
        } else {
            ss >> key;
            VctS vs = {cmd, key};
            res = it->second(std::move(vs));
        }
    } else if (cmd == "hset") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("command").toString();
        } else {
            ss >> key;
            ss >> objKey;
            ss >> objValue;
            VctS vs = {cmd, key, objKey, objValue};
            res = it->second(std::move(vs));
        }
    } else if (cmd == "hget") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("command").toString();
        } else {
            ss >> key;
            ss >> objKey;
            VctS vs = {cmd, key, objKey};
            res = it->second(std::move(vs));
        }
    } else if (cmd == "hgetall") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("command").toString();
        } else {
            ss >> key;
            VctS vs = {cmd, key};
            res = it->second(std::move(vs));
        }
    } else if (cmd == "sadd") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("command").toString();
        } else {
            ss >> key;
            ss >> objKey;
            VctS vs = {cmd, key, objKey};
            res = it->second(std::move(vs));
        }
    } else if (cmd == "smembers") {
        auto it = cmdDict.find(cmd);
        if (it == cmdDict.end()) {
            return dbStatus::notFound("command").toString();
        } else {
            ss >> key;
            VctS vs = {cmd, key};
            res = it->second(std::move(vs));
        }
    } else if(cmd == "zadd") {
        auto it = cmdDict.find(cmd);
        if(it == cmdDict.end()){
            return dbStatus::notFound("command").toString();
        }else{
            ss >> key;
            ss >> objKey;
            ss >> objValue;
            VctS vs = {cmd,key,objKey,objValue};
            res = it->second(std::move(vs));
        }
    }else if(cmd == "zcard"){
        auto it = cmdDict.find(cmd);
        if(it == cmdDict.end()){
            return dbStatus::notFound("command").toString();
        }else{
            ss >> key;
            VctS vs = {cmd,key};
            res = it->second(std::move(vs));
        }
    }else if(cmd == "zrange") {
        auto it = cmdDict.find(cmd);
        if(it == cmdDict.end()){
            return dbStatus::notFound("command").toString();
        }else{
            ss >> key;
            ss >> objKey;   // range start
            ss >> objValue; // range end
            VctS vs = {cmd,key,objKey,objValue};
            res = it->second(std::move(vs));
        }
    }else if(cmd == "zgetall"){
        auto it = cmdDict.find(cmd);
        if(it == cmdDict.end()){
            return dbStatus::notFound("command").toString();
        }else{
            ss >> key;
            VctS vs = {cmd,key};
            res = it->second(std::move(vs));
        }
    }else{
        return dbStatus::notFound("command").toString();
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
    // dbString
    bool res = database_[dbIndex]->setPExpireTime(dbObj::dbString, argv[1], atof(argv[2].c_str()));
    if (!res) {
        // dbList
        res = database_[dbIndex]->setPExpireTime(dbObj::dbList, argv[1], atof(argv[2].c_str()));
    }
    if(!res){
        // dbHash
        res = database_[dbIndex]->setPExpireTime(dbObj::dbHash,argv[1],atof(argv[2].c_str()));
    }
    if(!res){
        // dbSet
        res = database_[dbIndex]->setPExpireTime(dbObj::dbSet,argv[1],atof(argv[2].c_str()));
    }
    if(!res){
        // dbZSet
        res = database_[dbIndex]->setPExpireTime(dbObj::dbZSet,argv[1],atof(argv[2].c_str()));
    }

    return res ? dbStatus::Ok().toString() :
           dbStatus::IOError("pExpire error").toString();
}

std::string DBServer::expiredCommand(VctS &&argv) {
    if (argv.size() != 3) {
        return dbStatus::IOError("Parameter error").toString();
    }
    // dbString
    bool res = database_[dbIndex]->setPExpireTime(dbObj::dbString, argv[1],
                                                  atof(argv[2].c_str()) * Timestamp::kMicroSecondsPerMilliSecond);
    if (!res) {
        // dbList
        res = database_[dbIndex]->setPExpireTime(dbObj::dbList, argv[1],
                                                 atof(argv[2].c_str()) * Timestamp::kMicroSecondsPerMilliSecond);
    }
    if (!res) {
        //dbHash
        res = database_[dbIndex]->setPExpireTime(dbObj::dbHash, argv[1],
                                                 atof(argv[2].c_str()) * Timestamp::kMicroSecondsPerMilliSecond);
    }
    if (!res) {
        //dbSet
        res = database_[dbIndex]->setPExpireTime(dbObj::dbSet, argv[1],
                                                 atof(argv[2].c_str()) * Timestamp::kMicroSecondsPerMilliSecond);
    }
    if(!res){
        //dbZSet
        res = database_[dbIndex]->setPExpireTime(dbObj::dbZSet,argv[1],
                                                 atof(argv[2].c_str()) * Timestamp::kMicroSecondsPerMilliSecond);
    }
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

    return flag ? dbStatus::Ok().toString() : dbStatus::IOError("rpush error").toString();
}

std::string DBServer::rpopCommand(VctS &&argv) {
    if (argv.size() != 2) {
        return dbStatus::IOError("Parameter error").toString();
    }
    // 处理过期时间
    bool expired = database_[dbIndex]->judgeKeyExpiredTime(dbObj::dbList, argv[1]);
    if (expired) {
        database_[dbIndex]->delKey(dbObj::dbList, argv[1]);
        return dbStatus::IOError("Empty Content").toString();
    }

    std::string res = database_[dbIndex]->rpopList(argv[1]);
    if (res.empty()) {
        return dbStatus::IOError("rpop error").toString();
    } else {
        return res;
    }
}

std::string DBServer::hsetCommand(VctS &&argv) {
    if (argv.size() != 4) {
        return dbStatus::IOError("Parameter error").toString();
    }
    bool flag = database_[dbIndex]->addKey(dbObj::dbHash, argv[1], argv[2], argv[3]);

    return flag ? dbStatus::Ok().toString() : dbStatus::IOError("hset error").toString();
}

std::string DBServer::hgetCommand(VctS &&argv) {
    if (argv.size() != 3) {
        return dbStatus::IOError("Parameter error").toString();
    }
    auto tmp = database_[dbIndex]->getKeyHashObj();
    auto it = tmp.find(argv[1]);
    if (it == tmp.end()) {
        return dbStatus::notFound("Empty Content").toString();
    } else {
        auto iter = it->second.find(argv[2]);
        if (iter == it->second.end()) {
            return dbStatus::notFound("Empty Content").toString();
        } else {
            return iter->second;
        }
    }
}

std::string DBServer::hgetAllCommand(VctS &&argv) {
    if (argv.size() != 2) {
        return dbStatus::IOError("Parameter error").toString();
    }
    std::string res = database_[dbIndex]->getKey(dbObj::dbHash, argv[1]);

    return res.empty() ? dbStatus::IOError("Empty Content").toString() : res;
}

std::string DBServer::saddCommand(VctS &&argv) {
    if (argv.size() != 3) {
        return dbStatus::IOError("Parameter error").toString();
    }
    bool flag = database_[dbIndex]->addKey(dbObj::dbSet, argv[1], argv[2], dbObj::defaultObjValue);

    return flag ? dbStatus::Ok().toString() : dbStatus::IOError("sadd error").toString();
}

std::string DBServer::smembersCommand(VctS &&argv) {
    if (argv.size() != 2) {
        return dbStatus::IOError("Parameter error").toString();
    }
    std::string res = database_[dbIndex]->getKey(dbObj::dbSet,argv[1]);

    return res.empty() ? dbStatus::notFound("Empty Content").toString() : res;
}

std::string DBServer::zaddCommand(VctS &&argv) {
    if(argv.size() != 4){
        return dbStatus::IOError("Parameter error").toString();
    }
    bool flag = database_[dbIndex]->addKey(dbObj::dbZSet,argv[1],argv[2],argv[3]);

    return flag ? dbStatus::Ok().toString() : dbStatus::IOError("zadd error").toString();
}

std::string DBServer::zcardCommand(VctS &&argv) {
    if(argv.size() != 2){
        return dbStatus::IOError("Parameter error").toString();
    }
    auto tmpZset = database_[dbIndex]->getKeyZSetObj();
    auto it = tmpZset.find(argv[1]);
    if(it == tmpZset.end()){
        return dbStatus::notFound("Empty Content").toString();
    }else{
        return std::to_string(it->second->getLength());
    }
}

std::string DBServer::zrangeCommand(VctS &&argv) {
    if(argv.size() != 4){
        return dbStatus::IOError("Parameter error").toString();
    }
    std::string res;
    // zset的key
    std::string args = argv[1];
    // 添加range的范围
    args += ':' + argv[2] + '-' + argv[3];
    res = database_[dbIndex]->getKey(dbObj::dbZSet,args);

    return res.empty() ? dbStatus::notFound("Empty Content").toString() : res;
}

std::string DBServer::zgetAllCommand(VctS &&argv) {
    if(argv.size() != 2){
        return dbStatus::IOError("Parameter error").toString();
    }
    std::string res = database_[dbIndex]->getKey(dbObj::dbZSet,argv[1]);

    return res.empty() ? dbStatus::notFound("Empty Content").toString() : res;
}
std::string DBServer::saveHead() {
    std::string tmp = "KV0001";
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