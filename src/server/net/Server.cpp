//
// Created by wonder on 2021/9/24.
//

#include "Server.h"
#include "Acceptor.h"
#include "Logger.h"
#include "TcpConnection.h"

Server::Server(EventLoop *loop, const InetAddress &listenAddr, std::string name)
        : loop_(loop),
          name_(std::move(name)),
          acceptor_(new Acceptor(loop, listenAddr)),
          connectionCallback_(defaultConnectionCallback),
          messageCallback_(defaultMessageCallback),
          started_(false),
          nextConnId_(1){

    acceptor_->setNewConnectionCallback(
            std::bind(&Server::newConnection, this,
                      std::placeholders::_1,
                      std::placeholders::_2));

}

Server::~Server() {
    LOG_INFO("Server closed......\n");
}

void Server::start() {
    if (!started_) {
        started_ = true;
    }
    if (!acceptor_->listening()) {
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
    LOG_INFO("Server started......\n");
}

void Server::newConnection(int sockfd, const InetAddress &peerAddr) {
    loop_->assertInLoopThread();

    char buf[32];
    snprintf(buf, sizeof buf, "#%d", nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("Server::newConnection [%s] - new connection [%s] from %s\n",
             name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop_, connName,sockfd, localAddr, peerAddr);
    connections_[connName] = conn;

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);

    conn->setCloseCallback(
            std::bind(&Server::removeConnection, this, std::placeholders::_1));
    loop_->runInLoop(
            std::bind(&TcpConnection::connectEstablished, conn));
}

void Server::removeConnection(const TcpConnectionPtr &conn) {
    loop_->runInLoop(std::bind(&Server::removeConnectionInLoop, this, conn));
}

void Server::removeConnectionInLoop(const TcpConnectionPtr &conn) {
    loop_->assertInLoopThread();
    LOG_INFO("Server::removeConnection [%s] - connection.\n", conn->name().c_str());
    size_t n = connections_.erase(conn->name());
    assert(n == 1);
    loop_->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}