//
// Created by wonder on 2021/9/24.
//

#pragma once

#include <cstdint>
#include <string>
#include <netinet/in.h>

class InetAddress {
public:
  InetAddress(uint16_t port = 12345, std::string ip = "127.0.0.1");
  explicit InetAddress(const sockaddr_in &addr) : addr_(addr) {}

  std::string toIp() const;
  std::string toIpPort() const;
  uint16_t toPort() const;

  const sockaddr_in &getSockAddr() const { return addr_; }
  void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }

private:
  sockaddr_in addr_;
};