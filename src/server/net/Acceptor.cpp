//
// Created by wonder on 2021/9/25.
//

#include "Acceptor.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"
#include <unistd.h>

Acceptor::Acceptor(EventLoop *loop, const InetAddress &localAddr)
        : loop_(loop),
          localAddr_(localAddr),
          state_(kConnecting),
          acceptorSocket_(new Socket(sockets::createNonblockingOrDie())),
          acceptorChannel_(new Channel(loop_, acceptorSocket_->fd())),
          listening_(false) {

    acceptorSocket_->setReuseAddr(true);
    acceptorSocket_->setReusePort(true);
    acceptorSocket_->bindAddress(localAddr_);

    setNewConnectionCallback(std::bind(&Acceptor::newConnection,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2));
    acceptorChannel_->setReadCallback(std::bind(&Acceptor::handleAccept, this));
}

void Acceptor::newConnection(int sockfd, const InetAddress &peerAddr) {
    peerAddr_ = peerAddr;
    // 处理连接建立
    loop_->runInLoop(std::bind(&Acceptor::handleEstablished, shared_from_this()));
    // 设置连接断开回调
    shared_from_this()->setCloseCallback(std::bind(&Acceptor::handleClose, this));
    setChannel(std::make_shared<Channel>(loop_, sockfd));
}

void Acceptor::setChannel(const std::shared_ptr<Channel> &channel) {
    connChannel_ = channel;
    setState(StateE::kConnected);
    connChannel_->setReadCallback(std::bind(&Acceptor::handleRead, this));
    connChannel_->setWriteCallback(std::bind(&Acceptor::handleWrite, this));
    connChannel_->enableReading();
}

void Acceptor::removeChannel() {
    std::bind(&Acceptor::handleClose, this);
}

void Acceptor::listen() {
    listening_ = true;
    acceptorSocket_->listen();
    acceptorChannel_->enableReading();
}

void Acceptor::send(const std::string &message) {
    if (state_ == StateE::kConnected) {
        sendInLoop(message);
    }
}

void Acceptor::sendInLoop(const std::string &message) {
    ssize_t n_wrote = 0;
    if (state_ == StateE::kDisConnected) {
        LOG_ERROR("disconnected,give up writing!\n");
        return;
    }
    // 发送缓冲区没有数据
    if (!connChannel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        n_wrote = write(connChannel_->fd(), message.data(), message.size());
        if (n_wrote >= 0) {
//            LOG_INFO("write complete!\n");
        } else {
            n_wrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_FATAL("Acceptor::sendInLoop unknown error\n");
            }
        }
    }
    // 将未发生的数据放入发送缓冲区
    assert(n_wrote >= 0);
    if (static_cast<size_t>(n_wrote) < message.size()) {
        outputBuffer_.append(message.data() + n_wrote, message.size() - n_wrote);
        if (!connChannel_->isWriting()) {
            connChannel_->enableWriting();
        }
    }
}

void Acceptor::shutdown() {
    if (state_ == StateE::kConnected) {
        setState(StateE::kDisconnecting);
        loop_->runInLoop(std::bind(&Acceptor::shutdownInLoop, this));
    }
}

void Acceptor::shutdownInLoop() {
    if (!connChannel_->isWriting()) {
        acceptorSocket_->shutdownWrite();
    }
}

void Acceptor::handleAccept() {
    InetAddress peerAddr;
    int connfd = acceptorSocket_->accept(&peerAddr);
    if (connfd >= 0) {
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        } else {
            sockets::close(connfd);
        }
    }
}

void Acceptor::handleEstablished() {
    setState(StateE::kConnected);
    connectionCallback_(shared_from_this());
}

void Acceptor::handleRead() {
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(connChannel_->fd(), &savedErrno);
    if (n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, n);
    } else if (n == 0) {
        handleClose();
    } else {
        LOG_ERROR("Acceptor::handleRead error!\n");
        return;
    }
}

void Acceptor::handleWrite() {
    if (connChannel_->isWriting()) {
        ssize_t n = write(connChannel_->fd(),
                          outputBuffer_.peek(),
                          outputBuffer_.readableBytes());
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                connChannel_->disableWriting();
                if (state_ == StateE::kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            LOG_ERROR("Acceptor::handleWrite error\n");
        }
    } else {
        LOG_INFO("Connection fd = %d is down,no more writing", connChannel_->fd());
    }
}

void Acceptor::handleClose() {
    assert(state_ == StateE::kConnected || state_ == StateE::kDisconnecting);
    setState(StateE::kDisConnected);
    connChannel_->disableAll();
    connectionCallback_(shared_from_this());
    loop_->removeChannel(connChannel_.get());
}