//
// Created by wonder on 2021/9/20.
//

#include <sys/epoll.h>
#include <cstring>
#include <cassert>
#include "EpollPoller.h"
#include "Channel.h"
#include "Logger.h"
#include "EventLoop.h"

// Channel 未添加到poller中
const int kNew = -1;
// Channel已经添加到poller中
const int kAdded = 1;
// Channel已经从poller中删除
const int kDelete = 2;

EpollPoller::EpollPoller(EventLoop *loop)
    : epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize),
      ownerLoop_(loop) {

}

EpollPoller::~EpollPoller() {

}

void EpollPoller::assertInLoopThread() {
  ownerLoop_->assertInLoopThread();
}

Timestamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
  int numEvents = ::epoll_wait(epollfd_,
                               &*events_.begin(),
                               static_cast<int>(events_.size()),
                               timeoutMs);
  Timestamp now(Timestamp::now());
  if (numEvents > 0) {
    LOG_DEBUG("%d events happened.\n", numEvents);
    fillActiveChannels(numEvents, activeChannels);
    // 扩容
    if(numEvents == events_.size()){
      events_.resize(numEvents * 2);
    }
  } else if (numEvents == 0) {
    LOG_DEBUG(" %s timeout\n",__FUNCTION);
  } else {
    LOG_ERROR("EpollPoller:poll().\n");
  }
  return now;
}

void EpollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const {
  for (int i = 0; i < numEvents; i++) {
    Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
    int fd = channel->fd();
    auto it = channels_.find(fd);
    if (it == channels_.end() || it->second != channel) {
      return;
    }
    channel->set_revents(events_[i].events);
    activeChannels->push_back(channel);
  }
}

void EpollPoller::updateChannel(Channel *channel) {
  assertInLoopThread();
  const int index = channel->index();

  if (index == kNew || index == kDelete) {
    int fd = channel->fd();
    if (index == kNew) {
      if (channels_.find(fd) != channels_.end()) {
        LOG_ERROR("fd = %d must not exist in channels_.", fd);
        return;
      }
      channels_[fd] = channel;
    }
    channel->set_index(kAdded);
    update(EPOLL_CTL_ADD, channel);
  } else {
    int fd = channel->fd();
    if (channels_.find(fd) == channels_.end() || channels_[fd] != channel || index != kAdded) {
      LOG_ERROR("current channel is not matched current fd, fd = %d, channel = 0x%x", fd, channel);
      return;
    }
    if (channel->isNoneEvent()) {
      update(EPOLL_CTL_DEL, channel);
      channel->set_index(kDelete);
    } else {
      update(EPOLL_CTL_MOD, channel);
    }
  }
}

void EpollPoller::removeChannel(Channel *channel) {
  assertInLoopThread();
  assert(channels_.find(channel->fd()) != channels_.end());
  assert(channels_[channel->fd()] == channel);
  assert(channel->isNoneEvent());

  int idx = channel->index();
  assert(0 <= idx && idx < static_cast<int>(events_.size()));

  size_t n = channels_.erase(channel->fd());
  assert(n == 1);

  if(idx == kAdded){
    update(EPOLL_CTL_DEL,channel);
  }
  channel->set_index(kNew);
}

bool EpollPoller::update(int operation, Channel *channel) {
  epoll_event event;
  memset(&event, 0, sizeof event);

  int fd = channel->fd();
  event.events = channel->events();
  event.data.fd = fd;
  event.data.ptr = channel;

  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
    if (operation == EPOLL_CTL_DEL) {
      LOG_ERROR("epoll_ctl del error:%d\n", errno);
    } else {
      LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
    }
  }
}
