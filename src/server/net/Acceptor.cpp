//
// Created by wonder on 2021/9/25.
//

#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Logger.h"
#include <fcntl.h>
#include <unistd.h>

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr)
    :loop_(loop),
     acceptSocket_(sockets::createNonblockingOrDie()),
     acceptChannel_(loop_,acceptSocket_.fd()),
     listening_(false),
     idleFd_(::open("/dev/null",O_RDONLY | O_CLOEXEC)){

    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);

    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
}

void Acceptor::listen() {
    loop_->assertInLoopThread();
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead() {
    loop_->assertInLoopThread();
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0){
        if(newConnectionCallback_){
            newConnectionCallback_(connfd,peerAddr);
        }else{
            ::close(connfd);
        }
    }else{
        LOG_ERROR("%s:%s:%d accept socket create err:%d\n",__FILE__,__FUNCTION__,__LINE__,errno);
        if(errno == EMFILE){
            ::close(idleFd_);
            idleFd_ = ::accept(acceptSocket_.fd(), nullptr, nullptr);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null",O_RDONLY | O_CLOEXEC);
        }
    }
}