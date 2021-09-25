//
// Created by wonder on 2021/9/24.
//

#include "Socket.h"

#include "Logger.h"
#include "InetAddress.h"

#include <unistd.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <cstring>
#include <fcntl.h>

Socket::~Socket() {
  ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localAddr) {
  sockets::bind(sockfd_,localAddr.getSockAddr());
}

void Socket::listen() {
  int ret = ::listen(sockfd_, 1024);
  if (ret != 0) {
    LOG_FATAL("listen sockfd:%d fail\n", sockfd_);
  }
}

int Socket::accept(InetAddress *peerAddr) {
  sockaddr_in addr;
  socklen_t len;

  bzero(&addr, sizeof addr);
  len = sizeof(addr);

  int connfd = ::accept4(sockfd_, (sockaddr *) &addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (connfd >= 0) {
    peerAddr->setSockAddr(addr);
  }
  return connfd;
}

void Socket::shutdownWrite() {
  if (::shutdown(sockfd_, SHUT_WR) < 0) {
    LOG_ERROR("shutdownWrite error.");
  }
}

void Socket::setTcpNoDelay(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}

void Socket::setReuseAddr(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}

void Socket::setReusePort(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}

void Socket::setKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}

int sockets::createNonblockingOrDie() {
  int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    LOG_FATAL("%s:%s:%d listen socket create err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
  }
  setNonBlockAndCloseOnExec(sockfd);
  return sockfd;
}
void sockets::setNonBlockAndCloseOnExec(int sockfd){
  int flags = ::fcntl(sockfd,F_GETFL,0);
  flags |= O_NONBLOCK;
  int ret = ::fcntl(sockfd,F_SETFL,flags);

  flags = ::fcntl(sockfd,F_GETFD,0);
  flags |= FD_CLOEXEC;
  ret = ::fcntl(sockfd,F_SETFD,flags);
}

int sockets::connect(int sockfd, const sockaddr_in &addr) {
  return ::connect(sockfd, sockaddr_cast(&addr), sizeof addr);
}

void sockets::bind(int sockfd, const sockaddr_in &addr) {
  int ret = ::bind(sockfd, sockaddr_cast(&addr),sizeof addr);
  if(ret < 0){
    LOG_FATAL("sockets::bind");
  }
}

bool sockets::isSelfConnect(int sockfd) {
  struct sockaddr_in localAddr = getLocalAddr(sockfd);
  struct sockaddr_in peerAddr = getPeerAddr(sockfd);
  return localAddr.sin_port == peerAddr.sin_port && localAddr.sin_addr.s_addr == peerAddr.sin_addr.s_addr;
}

void sockets::close(int sockfd) {
  ::close(sockfd);
}

sockaddr *sockets::sockaddr_cast(sockaddr_in *addr) {
  return static_cast<sockaddr *>((void *) (addr));
}

const sockaddr *sockets::sockaddr_cast(const sockaddr_in *addr) {
  return static_cast<const sockaddr *>((const void *) (addr));
}

sockaddr_in sockets::getLocalAddr(int sockfd) {

  sockaddr_in localAddr = {0};
  memset(&localAddr, 0, sizeof localAddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof localAddr);
  ::getsockname(sockfd, sockaddr_cast(&localAddr), &addrlen);
  return localAddr;
}

sockaddr_in sockets::getPeerAddr(int sockfd) {
  struct sockaddr_in peerAddr = {0};
  memset(&peerAddr, 0, sizeof peerAddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof peerAddr);
  ::getpeername(sockfd, sockaddr_cast(&peerAddr), &addrlen);
  return peerAddr;
}

int sockets::getSocketError(int sockfd) {
  int optval;
  socklen_t optlen = static_cast<socklen_t>(sizeof optval);
  if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
    return errno;
  }
  return optval;
}