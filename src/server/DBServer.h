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

private:
    void initDB();

    std::string parseMsg(const std::string &msg);

    std::string setCommand(VctS&&);
    std::string getCommand(VctS&&);

    // db相关
    std::unique_ptr<Database> database_;
    std::unordered_map<std::string,
            std::function<std::string(VctS &&)>> cmdDict;

    // net相关
    EventLoop *loop_;
    Server server_;
};