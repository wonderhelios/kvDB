//
// Created by wonder on 2021/9/24.
//

#pragma once
#include "Callbacks.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include <functional>
#include <map>

class EventLoop;
class InetAddress;

class Server {
public:
    Server(EventLoop *loop,const InetAddress & listenAddr,
           std::string name);
    ~Server();

    void start();

    void setConnectionCallback(const ConnectionCallback & cb){ connectionCallback_ = cb;}
    void setMessageCallback(const MessageCallback & cb){ messageCallback_ = cb;}

private:
    void newConnection(int sockfd,const InetAddress & peerAddr);
    void removeConnection(const TcpConnectionPtr & conn);
    void removeConnectionInLoop(const TcpConnectionPtr & conn);

    using ConnectionMap = std::map<std::string,TcpConnectionPtr>;

    EventLoop * loop_;
    std::unique_ptr<Acceptor> acceptor_;
    const std::string name_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;

    bool started_;
    int nextConnId_;
    ConnectionMap connections_;
};
