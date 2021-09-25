//
// Created by wonder on 2021/9/24.
//

#pragma once

#include "noncopyable.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

class InetAddress;

class Socket : noncopyable{
public:
  explicit Socket(int sockfd)
      :sockfd_(sockfd){
  }
  ~Socket();

  int fd() const{
    return sockfd_;
  }
  void bindAddress(const InetAddress & localAddr);
  void listen();
  int accept(InetAddress * peerAddr);

  void shutdownWrite();

  void setTcpNoDelay(bool on);
  void setReuseAddr(bool on);
  void setReusePort(bool on);
  void setKeepAlive(bool on);

private:
  const int sockfd_;
};

namespace sockets{

int createNonblockingOrDie();
void setNonBlockAndCloseOnExec(int sockfd);
int connect(int sockfd,const sockaddr_in & addr);
void bind(int sockfd,const sockaddr_in & addr);
bool isSelfConnect(int sockfd);
void close(int sockfd);
sockaddr* sockaddr_cast(sockaddr_in* addr);
const sockaddr* sockaddr_cast(const sockaddr_in* addr);
sockaddr_in getLocalAddr(int sockfd);
sockaddr_in getPeerAddr(int sockfd);
int getSocketError(int sockfd);
}