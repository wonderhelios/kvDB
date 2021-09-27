//
// Created by wonder on 2021/9/26.
//

#pragma once

#include <ext/pool_allocator.h>
#include <memory>
#include <unordered_map>
#include <string>
#include "../net/Timestamp.h"

template<typename T1, typename T2>
using Dict = std::unordered_map<T1, T2, std::hash<T1>,
        std::equal_to<T1>,
        __gnu_cxx::__pool_alloc<std::pair<const T1, T2>>>;

typedef Dict<std::string, std::string> String;
typedef Dict<std::string, Timestamp> Expire;

class Database {
public:
    Database() {}

    ~Database() {}

    bool addKey(const int type, const std::string &key, const std::string &objKey,
                const std::string &objValue);

    bool delKey(const int type, const std::string &key);

    std::string getKey(const int type, const std::string &key);

    bool setPExpireTime(const int type, const std::string &key, double expiredTime);

    Timestamp getKeyExpiredTime(const int type, const std::string &key);

    bool judgeKeyExpiredTime(const int type, const std::string &key);

private:
    String String_;

    Expire StringExpire_;
};