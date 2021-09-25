//
// Created by wonder on 2021/9/24.
//

#pragma once
#include "Callbacks.h"
#include "EventLoop.h"
#include <functional>

class EventLoop;
class InetAddress;

class Server {
public:
    Server(EventLoop *loop,const InetAddress & listenAddr);
    ~Server();

    void start();

    void onConnection(const AcceptorPtr &);
    void onMessage(const AcceptorPtr &,
                   Buffer *buf,
                   ssize_t n);
private:
    EventLoop * loop_;
    AcceptorPtr acceptor_;
};
