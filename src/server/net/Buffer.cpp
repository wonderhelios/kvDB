//
// Created by wonder on 2021/7/19.
//

#include <sys/uio.h>
#include "Buffer.h"

const char Buffer::kCRLF[] = "\r\n";

Buffer::Buffer()
    :buffer_(kCheapPrepend + kInitialSize),
    readerIndex_(kCheapPrepend),
    writerIndex_(kCheapPrepend){

    assert(readableBytes() == 0);
    assert(writableBytes() == kInitialSize);
    assert(prependableBytes() == kCheapPrepend);
}

void Buffer::swap(Buffer &rhs) {
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_,rhs.readerIndex_);
    std::swap(writerIndex_,rhs.writerIndex_);
}

ssize_t Buffer::readFd(int fd, int *savedErrno) {
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const ssize_t n = readv(fd,vec,2);
    if(n < 0){
        *savedErrno = errno;
    }else if(size_t(n) <= writable){
        writerIndex_ += n;
    }else{
        writerIndex_ = buffer_.size();
        append(extrabuf,n - writable);
    }
    return n;
}