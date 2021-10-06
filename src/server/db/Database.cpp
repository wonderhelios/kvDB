//
// Created by wonder on 2021/9/26.
//

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cassert>
#include <sys/stat.h>
#include <sys/mman.h>
#include "Database.h"
#include "dbObj.h"
#include "dbStatus.h"
#include "../net/Logger.h"

void Database::rdbLoad(int index) {
    char tmp[1024]{0};
    std::string path = getcwd(tmp,1024);
    path += "/dump.rdb";
    int fd = open(path.c_str(),O_CREAT | O_RDONLY,0644);
    assert(fd != -1);

    // 获取文件信息
    struct stat buf;
    fstat(fd,&buf);
    if(buf.st_size == 0) return;

    char * addr = static_cast<char *>(mmap(NULL,buf.st_size,PROT_READ,MAP_SHARED,fd,0));
    if(addr == MAP_FAILED){
        close(fd);
        LOG_FATAL("rdbSave error");
    }
    close(fd);

    std::string data(addr,addr + buf.st_size);
    assert(munmap(addr,buf.st_size) != -1);

    int p1 = 0,p2 = 0;
    int dbIdx = 0;
    do{
        p1 = data.find("SD",p2);
        // 没有数据需要载入
        if(p1 == data.npos){
            return;
        }
        p2 = data.find('^',p1);
        dbIdx = atoi(interceptString(data,p1 + 2,p2).c_str());
    }while(dbIdx != index);

    int end = data.find("SD",p2);
    if(end == data.npos){
        end = data.find("EOF");
    }

    while(p1 < end && p2 < end){
        p2 = data.find('^',p1);
        p1 = data.find("ST",p2);
        int type = atoi(interceptString(data,p2+1,p1).c_str());

        if(type == dbObj::dbString){
            do{
                p2 = data.find('!',p1);
                Timestamp expireTime(atoi(interceptString(data,p1 + 2,p2).c_str()));
                p1 = data.find('#',p2);

                int keyLen = atoi(interceptString(data,p2+1,p1).c_str());
                std::string key = data.substr(p1+1,keyLen);

                p2 = data.find('!',p1);
                p1 = data.find('$',p2);
                int valueLen = atoi(interceptString(data,p2+1,p1).c_str());
                std::string value = data.substr(p1+1,valueLen);

                addKey(dbObj::dbString,key,value,dbObj::defaultObjValue);
                if(expireTime > Timestamp::now()){
                    setPExpireTime(dbObj::dbString,key,expireTime);
                }
                p1 += valueLen + 1;
            } while (data.substr(p1,2) == "ST");
            continue;
        }
    }
}

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
    }else if(type == dbObj::dbList){
        auto it = List_.find(key);
        if(it == List_.end()){
            std::list<std::string,__gnu_cxx::__pool_alloc<std::string>> tmp;
            tmp.emplace_back(objKey);
            List_.insert(std::make_pair(key,tmp));
        }else{
            it->second.emplace_back(objKey);
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

bool Database::setPExpireTime(const int type, const std::string &key, double expiredTime /* milliSeconds*/) {
    if(type == dbObj::dbString){
        auto it = String_.find(key);
        if(it != String_.end()){
            auto now = addTime(Timestamp::now(),expiredTime / Timestamp::kMilliSecondsPerSecond);
            StringExpire_[key] = now;
            return true;
        }
    }
    return false;
}

bool Database::setPExpireTime(const int type, const std::string &key, const Timestamp &expiredTime) {
    double expired_time = expiredTime.microSecondsSinceEpoch() / Timestamp::kMicroSecondsPerMilliSecond;
    return setPExpireTime(type,key,expired_time);
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

const std::string Database::rpopList(const std::string &key) {
    auto iter = List_.find(key);
    if(iter != List_.end()){
        if(iter->second.empty()){
            return dbStatus::notFound("nil").toString();
        }
        std::string res = iter->second.back();
        iter->second.pop_back();
        return res;
    }else{
        return dbStatus::notFound("Not Found").toString();
    }
}

std::string Database::interceptString(const std::string &ss, int p1, int p2) {
    if(p1 > p2){
        std::swap(p1,p2);
    }
    return ss.substr(p1,p2 - p1);
}