//
// Created by wonder on 2021/9/26.
//

#pragma once

#include "net/EventLoop.h"
#include "net/Server.h"
#include "net/InetAddress.h"
#include "db/Database.h"

class DBServer {
public:
    typedef std::vector<std::string> VctS;

    DBServer(EventLoop *loop, const InetAddress &localAddr);

    ~DBServer() {}

    void onConnection(const TcpConnectionPtr &);

    void onMessage(const TcpConnectionPtr &,
                   Buffer *buf,
                   Timestamp);

    void start();

    void rdbSave();
private:
    // 数据分库的数目
    static const long DEFAULT_DB_NUM = 16;

    void initDB();

    std::string parseMsg(const std::string &msg);

    std::string setCommand(VctS&&);
    std::string getCommand(VctS&&);
    std::string pExpiredCommand(VctS&&);
    std::string expiredCommand(VctS&&);
    std::string bgsaveCommand(VctS&&);
    std::string selectCommand(VctS&&);
    std::string rpushCommand(VctS&&);
    std::string rpopCommand(VctS&&);
    std::string hsetCommand(VctS&&);
    std::string hgetCommand(VctS&&);
    std::string hgetAllCommand(VctS&&);
    std::string saddCommand(VctS&&);
    std::string smembersCommand(VctS&&);

    std::string saveHead();
    std::string saveSelectDB(const int index);
    std::string saveExpiredTime(const Timestamp expiredTime);
    std::string saveType(const int type);
    std::string saveKV(const std::string & key,const std::string & value);

    bool checkSaveCondition();

    // db相关
    std::vector<std::unique_ptr<Database>> database_;
    int dbIndex;
    std::unordered_map<std::string,
            std::function<std::string(VctS &&)>> cmdDict;
    Timestamp lastSave_;
    // net相关
    EventLoop *loop_;
    Server server_;
};