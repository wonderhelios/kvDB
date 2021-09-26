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
        }else{
            std::cout<<"Not Found"<<std::endl;
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