//
// Created by wonder on 2021/9/25.
//

#pragma once

#include <functional>
#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

class EventLoop;
class InetAddress;

class Acceptor : noncopyable{
public:
    using NewConnectionCallback = std::function<void(int sockfd,const InetAddress&)>;

    Acceptor(EventLoop * loop,const InetAddress & listenAddr);

    void setNewConnectionCallback(const NewConnectionCallback & cb){
        newConnectionCallback_ = cb;
    }

    void listen();

    bool listening() const{
        return listening_;
    }
private:
    void handleRead();

    EventLoop * loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;

    NewConnectionCallback newConnectionCallback_;
    bool listening_;
    // 防文件描述符耗尽
    int idleFd_;
};