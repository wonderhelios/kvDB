//
// Created by wonder on 2021/9/24.
//

#include <sys/wait.h>
#include "Server.h"
#include "Acceptor.h"
#include "Logger.h"

Server::Server(EventLoop *loop, const InetAddress &listenAddr)
        : loop_(loop),
          acceptor_(new Acceptor(loop_, listenAddr)){

    acceptor_->setConnectionCallback(std::bind(&Server::onConnection,
                                               this,std::placeholders::_1));
    acceptor_->setMessageCallback(std::bind(&Server::onMessage,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            std::placeholders::_3));
}
Server::~Server() {
    LOG_INFO("Server closed...\n");
    while (waitpid(-1, nullptr, WNOHANG) != -1);
}

void Server::start() {
    LOG_INFO("Server started...\n");
    acceptor_->listen();
}

void Server::onConnection(const AcceptorPtr & acceptor) {
    if(acceptor_->connected()) {
        LOG_INFO("Connection UP!!\n");
    }else{
        LOG_INFO("Connection DOWN!!\n");
    }
}

void Server::onMessage(const AcceptorPtr & acceptor, Buffer *buf, ssize_t n) {
    auto msg = buf->retrieveAsString();
    LOG_INFO("msg: %s",msg.c_str());
    acceptor->send(msg);
}