//
// Created by wonder on 2021/9/26.
//

#include <iostream>
#include "Database.h"
#include "dbObj.h"
#include "dbStatus.h"

bool Database::addKey(const int type, const std::string &key, const std::string &objKey,
                      const std::string &objValue) {
    // 若为字符串类型
    if(type == dbObj::dbString){
        auto it = String_.find(key);
        if(it == String_.end()){
            String_.insert(std::make_pair(key,objKey));
        }else{
            String_[key] = objKey;
        }
    }
}

bool Database::delKey(const int type, const std::string &key) {
    if(type == dbObj::dbString){
        auto it = String_.find(key);
        if(it != String_.end()){
            String_.erase(key);
            StringExpire_.erase(key);
        }else{
//            std::cout<<"Not Found"<<std::endl;
            return false;
        }
    }
}

std::string Database::getKey(const int type, const std::string &key) {
    std::string res;

    if(type == dbObj::dbString){
        auto it = String_.find(key);
        if(it == String_.end()){
            res = dbStatus::notFound("Not Found").toString();
        }else{
            res = '+' + it->second;
        }
    }
    return res;
}

bool Database::setPExpireTime(const int type, const std::string &key, double expiredTime) {
    if(type == dbObj::dbString){
        auto it = String_.find(key);
        if(it != String_.end()){
            auto now = addTime(Timestamp::now(),expiredTime / Timestamp::kMicroSecondsPerMilliSecond);
            StringExpire_[key] = now;
            return true;
        }
    }
    return false;
}

Timestamp Database::getKeyExpiredTime(const int type, const std::string &key) {
    Timestamp tmp;
    if(type == dbObj::dbString){
        auto it = StringExpire_.find(key);
        if(it != StringExpire_.end()){
            tmp = it->second;
        }else{
            tmp = Timestamp::invalid();
        }
    }
    return tmp;
}

bool Database::judgeKeyExpiredTime(const int type, const std::string &key) {
    Timestamp expired = getKeyExpiredTime(type,key);
    if(expired == Timestamp::invalid()){
        return false;
    }else{
        Timestamp now = Timestamp::now();
        if(now > expired){
            return true;
        }else{
            return false;
        }
    }
}