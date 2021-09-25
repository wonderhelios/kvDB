//
// Created by wonder on 2021/9/24.
//

#pragma once

#include "Timestamp.h"
#include <functional>
#include <memory>

class Acceptor;
class Buffer;
class InetAddress;

using AcceptorPtr = std::shared_ptr<Acceptor>;

using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;
using ConnectionCallback = std::function<void(const AcceptorPtr &)>;
using MessageCallback = std::function<void(const AcceptorPtr &,
                                           Buffer *buf,
                                           ssize_t n)>;
using CloseCallback = std::function<void()>;
using ErrorCallback = std::function<void()>;