//
// Created by wonder on 2021/9/25.
//

#pragma once

#include <memory>
#include "noncopyable.h"
#include "Callbacks.h"
#include "InetAddress.h"
#include "Buffer.h"

class EventLoop;

class Channel;

class Socket;

class TcpConnection :
        public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(EventLoop * loop,
                  std::string & name,
                  int sockfd,
                  const InetAddress & localAddr,
                  const InetAddress & peerAddr);
    ~TcpConnection();

    EventLoop * getLoop() const{ return loop_;}
    const std::string & name() const { return name_;}
    const InetAddress & localAddr() const{ return localAddr_;};
    const InetAddress & peerAddr() const{ return peerAddr_;}
    bool connected() const{ return state_ == kConnected;}

    void setConnectionCallback(const ConnectionCallback & cb){ connectionCallback_ = cb;}
    void setMessageCallback(const MessageCallback & cb){ messageCallback_ = cb;}
    void setWriteCompletedCallback(const WriteCompleteCallback & cb){ writeCompleteCallback_ = cb;}
    void setCloseCallback(const CloseCallback & cb){ closeCallback_ = cb;}

    // 当接收了一个新连接后调用.
    void connectEstablished();
    // 从TCPServer的map中删除
    void connectDestroyed();

    void send(const std::string & message);

    void shutdown();
    void setTcpNoDelay(bool on);
    void setKeepAlive(bool on);

private:
    enum StateE{
        kConnecting,kConnected,kDisconnecting,kDisConnected,
    };
    void setState(StateE s){ state_ = s;}
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const std::string & message);
    void shutdownInLoop();

    EventLoop * loop_;
    std::string name_;
    StateE state_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    InetAddress localAddr_;
    InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    CloseCallback closeCallback_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;
};

