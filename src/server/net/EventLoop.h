//
// Created by wonder on 2021/9/20.
//

#pragma once

#include <atomic>
#include <functional>
#include <thread>
#include <vector>
#include "Timestamp.h"

class EpollPoller;
class Channel;

class EventLoop {
public:
  using Functor = std::function<void()>;

  EventLoop();
  ~EventLoop();

  void loop();
  void quit();

  void assertInLoopThread(){
    if(!isInLoopThread()){
      abortNotInLoopThread();
    }
  }
  bool isInLoopThread() const{
    return threadId_ == std::this_thread::get_id();
  }

  void runInLoop(const Functor & cb);

  void updateChannel(Channel * channel);
  void removeChannel(Channel * channel);

private:
  void abortNotInLoopThread();

  using ChannelList = std::vector<Channel*>;

  std::atomic_bool looping_;
  std::atomic_bool quit_;
  const std::thread::id threadId_;
  Timestamp epollReturnTime_;

  std::unique_ptr<EpollPoller> poller_;
  ChannelList activeChannels_;
};
