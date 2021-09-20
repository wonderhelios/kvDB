//
// Created by wonder on 2021/7/10.
//

#pragma once

class noncopyable{
protected:
    noncopyable() = default;
    ~noncopyable() = default;

public:
    noncopyable(const noncopyable &) = delete;
    noncopyable & operator=(const noncopyable &) = delete;
};
