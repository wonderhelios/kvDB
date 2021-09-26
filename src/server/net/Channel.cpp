//
// Created by wonder on 2021/9/20.
//

#include <sys/epoll.h>
#include <cassert>
#include "Channel.h"
#include "EventLoop.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    :loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1){

}
Channel::~Channel() {
}

void Channel::update() {
  loop_->updateChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime) {
  if(revents_ & EPOLLERR){
    if(errorCallback_) errorCallback_();
  }
  if(revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)){
    if(readCallback_) readCallback_(receiveTime);
  }
  if(revents_ & EPOLLOUT){
    if(writeCallback_) writeCallback_();
  }
}

void Channel::enableReading() {
  events_ |= kReadEvent;
  update();
}
void Channel::enableWriting() {
  events_ |= kWriteEvent;
  update();
}
void Channel::disableWriting() {
  events_ &= ~kWriteEvent;
  update();
}
void Channel::disableAll() {
  events_ = kNoneEvent;
  update();
}
