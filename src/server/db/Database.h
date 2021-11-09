//
// Created by wonder on 2021/9/26.
//

#pragma once

#include <ext/pool_allocator.h>
#include <memory>
#include <unordered_map>
#include <string>
#include <list>
#include <map>
#include <unordered_set>
#include "../net/Timestamp.h"
#include "skipList.h"

typedef std::shared_ptr<SkipList> SP_SkipList;

template<typename T1, typename T2>
using Dict = std::unordered_map<T1, T2, std::hash<T1>,
        std::equal_to<T1>,
        __gnu_cxx::__pool_alloc<std::pair<const T1, T2>>>;


typedef Dict<std::string, std::string> String;
typedef Dict<std::string, std::list<std::string, __gnu_cxx::__pool_alloc<std::string>>> List;
typedef Dict<std::string, std::map<std::string, std::string, std::less<>, __gnu_cxx::__pool_alloc<std::pair<const std::string, std::string>>>> Hash;
typedef Dict<std::string, std::unordered_set<std::string, std::hash<std::string>, std::equal_to<>, __gnu_cxx::__pool_alloc<std::string>>> Set;
typedef Dict<std::string, SP_SkipList> ZSet;

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

    bool setPExpireTime(const int type, const std::string &key, const Timestamp &expiredTime);

    Timestamp getKeyExpiredTime(const int type, const std::string &key);

    bool judgeKeyExpiredTime(const int type, const std::string &key);

    const std::string rpopList(const std::string &key);

    String &getKeyStringObj() {
        return String_;
    }

    List &getKeyListObj() {
        return List_;
    }

    Hash &getKeyHashObj() {
        return Hash_;
    }

    Set &getKeySetObj() {
        return Set_;
    }

    ZSet &getKeyZSetObj(){
        return ZSet_;
    }

    // 得到当前数据库键的数目
    int getKeySize() const {
        return getKeyStringSize() + getKeyListSize() + getKeyHashSize() + getKeySetSize() + getKeyZSetSize();
    }

    int getKeyStringSize() const {
        return String_.size();
    };

    int getKeyListSize() const {
        return List_.size();
    }

    int getKeyHashSize() const {
        return Hash_.size();
    }

    int getKeySetSize() const {
        return Set_.size();
    }

    int getKeyZSetSize() const {
        return ZSet_.size();
    }

private:

    std::string interceptString(const std::string &ss, int p1, int p2);

    String String_;
    List List_;
    Hash Hash_;
    Set Set_;
    ZSet ZSet_;

    Expire StringExpire_;
    Expire ListExpire_;
    Expire HashExpire_;
    Expire SetExpire_;
    Expire ZSetExpire_;
};