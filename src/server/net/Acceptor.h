//
// Created by wonder on 2021/9/25.
//

#pragma once

#include "Acceptor.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Socket.h"
#include "Channel.h"
#include <memory>

class EventLoop;

class Acceptor : public std::enable_shared_from_this<Acceptor>{
public:
    Acceptor(EventLoop * loop,const InetAddress & localAddr);

    EventLoop * getLoop() const{ return loop_;}
    void newConnection(int sockfd,const InetAddress & peerAddr);
    void listen();
    bool listening() const{return listening_;};
    bool connected() const{ return state_ == kConnected;}

    void setChannel(const std::shared_ptr<Channel> & channel);
    void removeChannel();

    void setNewConnectionCallback(const NewConnectionCallback & cb){newConnectionCallback_ = cb;}
    void setConnectionCallback(const ConnectionCallback & cb){ connectionCallback_ = cb;}
    void setMessageCallback(const MessageCallback & cb){ messageCallback_ = cb;}
    void setCloseCallback(const CloseCallback & cb){ closeCallback_ = cb;}

    void send(const std::string & message);
    void shutdown();

    std::string getSocketInfo(){ return peerAddr_.toIpPort();}
private:
    enum StateE{
        kConnecting,kConnected,kDisconnecting,kDisConnected,
    };

    void setState(StateE s){ state_ = s;}

    void handleAccept();
    void handleEstablished();
    void handleRead();
    void handleWrite();
    void handleClose();

    void sendInLoop(const std::string & message);
    void shutdownInLoop();

    EventLoop * loop_;
    InetAddress localAddr_;
    InetAddress peerAddr_;
    StateE state_;
    // acceptor对象本身
    std::unique_ptr<Socket> acceptorSocket_;
    std::unique_ptr<Channel> acceptorChannel_;
    // 连接acceptor对象的Channel
    std::shared_ptr<Channel> connChannel_;
    bool listening_;

    NewConnectionCallback newConnectionCallback_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    CloseCallback closeCallback_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;
};