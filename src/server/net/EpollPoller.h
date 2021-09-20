//
// Created by wonder on 2021/9/20.
//

#pragma once

#include <vector>
#include <map>
#include "noncopyable.h"
#include "Timestamp.h"

struct epoll_event;

class Channel;
class EventLoop;

class EpollPoller : noncopyable{
public:
  using ChannelList = std::vector<Channel*>;

  explicit EpollPoller(EventLoop * loop);
  ~EpollPoller();

  Timestamp poll(int timeoutMs,ChannelList * activeChannels);

  void updateChannel(Channel * channel);
  void removeChannel(Channel * channel);

  void assertInLoopThread();

private:
  void fillActiveChannels(int numEvents,
                          ChannelList * activeChannels) const;
  bool update(int operation,Channel * channel);

  static const int kInitEventListSize = 16;
  using EventList = std::vector<struct epoll_event>;
  using ChannelMap = std::map<int,Channel *>;

  EventLoop * ownerLoop_;

  int epollfd_;
  // 缓存epoll_event的数组
  EventList events_;
  // fd到Channel的映射
  ChannelMap channels_;
};
