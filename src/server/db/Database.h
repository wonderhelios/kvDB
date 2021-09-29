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

    void rdbLoad(int index);

    bool addKey(const int type, const std::string &key, const std::string &objKey,
                const std::string &objValue);

    bool delKey(const int type, const std::string &key);

    std::string getKey(const int type, const std::string &key);

    bool setPExpireTime(const int type, const std::string &key, double expiredTime);
    bool setPExpireTime(const int type, const std::string &key, const Timestamp & expiredTime);

    Timestamp getKeyExpiredTime(const int type, const std::string &key);

    bool judgeKeyExpiredTime(const int type, const std::string &key);

    String & getKeyStringObj() {
        return String_;
    }
    // 得到当前数据库键的数目
    int getKeySize() const{
        return getKeyStringSize();
    }
    int getKeyStringSize() const {
        return String_.size();
    };
private:

    std::string interceptString(const std::string & ss,int p1,int p2);

    String String_;

    Expire StringExpire_;
};