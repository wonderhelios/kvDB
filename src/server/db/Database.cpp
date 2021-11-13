//
// Created by wonder on 2021/9/26.
//

#include <unistd.h>
#include <fcntl.h>
#include <cassert>
#include <sys/stat.h>
#include <sys/mman.h>
#include <cfloat>
#include "Database.h"
#include "dbObj.h"
#include "dbStatus.h"
#include "../net/Logger.h"

void Database::rdbLoad(int index) {
    char tmp[1024]{0};
    std::string path = getcwd(tmp, 1024);
    path += "/dump.rdb";
    int fd = open(path.c_str(), O_CREAT | O_RDONLY, 0644);
    assert(fd != -1);

    // 获取文件信息
    struct stat buf;
    fstat(fd, &buf);
    if (buf.st_size == 0) return;

    char *addr = static_cast<char *>(mmap(NULL, buf.st_size, PROT_READ, MAP_SHARED, fd, 0));
    if (addr == MAP_FAILED) {
        close(fd);
        LOG_FATAL("rdbSave error");
    }
    close(fd);

    std::string data(addr, addr + buf.st_size);
    assert(munmap(addr, buf.st_size) != -1);

    int p1 = 0, p2 = 0;
    int dbIdx = 0;
    do {
        p1 = data.find("SD", p2);
        // 没有数据需要载入
        if (p1 == data.npos) {
            return;
        }
        p2 = data.find('^', p1);
        dbIdx = atoi(interceptString(data, p1 + 2, p2).c_str());
    } while (dbIdx != index);

    int end = data.find("SD", p2);
    if (end == data.npos) {
        end = data.find("EOF");
    }

    while (p1 < end && p2 < end) {
        p2 = data.find('^', p1);
        p1 = data.find("ST", p2);
        int type = atoi(interceptString(data, p2 + 1, p1).c_str());

        if (type == dbObj::dbString) {
            do {
                p2 = data.find('!', p1);
                Timestamp expireTime(atoi(interceptString(data, p1 + 2, p2).c_str()));
                p1 = data.find('#', p2);

                int keyLen = atoi(interceptString(data, p2 + 1, p1).c_str());
                std::string key = data.substr(p1 + 1, keyLen);

                p2 = data.find('!', p1);
                p1 = data.find('$', p2);
                int valueLen = atoi(interceptString(data, p2 + 1, p1).c_str());
                std::string value = data.substr(p1 + 1, valueLen);

                addKey(dbObj::dbString, key, value, dbObj::defaultObjValue);
                if (expireTime > Timestamp::now()) {
                    setPExpireTime(dbObj::dbString, key, expireTime);
                }
                p1 += valueLen + 1;
            } while (data.substr(p1, 2) == "ST");
            continue;
        }
        if (type == dbObj::dbList) {
            do {
                p2 = data.find('!', p1);
                Timestamp expireTime(atoi(interceptString(data, p1 + 2, p2).c_str()));
                p1 = data.find('#', p2);

                int keyLen = atoi(interceptString(data, p2 + 1, p1).c_str());
                std::string key = data.substr(p1 + 1, keyLen);

                p2 = data.find('!', p1);
                p1 = data.find('$', p2);
                int valueSize = atoi(interceptString(data, p2 + 1, p1).c_str());
                int valueLen = 0;
                while (valueSize--) {
                    p2 = p1;
                    p1 = data.find('$', p2);
                    valueLen = atoi(interceptString(data, p2 + 1, p1).c_str());
                    std::string value = data.substr(p1 + 1, valueLen);
                    if (valueSize > 1) {
                        p1 = data.find('!', p2 + 1);
                    }
                    addKey(dbObj::dbList, key, value, dbObj::defaultObjValue);
                }
                if (expireTime > Timestamp::now()) {
                    setPExpireTime(dbObj::dbList, key, expireTime);
                }
                p1 += valueLen + 1;
            } while (data.substr(p1, 2) == "ST");
            continue;
        }
        if (type == dbObj::dbHash) {
            do {
                p2 = data.find('!', p1);
                Timestamp expireTime(atoi(interceptString(data, p1 + 2, p2).c_str()));
                p1 = data.find('#', p2);
                int keyLen = atoi(interceptString(data, p2 + 1, p1).c_str());
                std::string key = data.substr(p1 + 1, keyLen);
                p2 = data.find('!', p1);
                p1 = data.find('!', p2 + 1);
                int valueSize = atoi(interceptString(data, p2 + 1, p1).c_str());
                int valueLen = 0;
                while (valueSize--) {
                    p2 = data.find('#', p1);
                    int valueKeyLen = atoi(interceptString(data, p1 + 1, p2).c_str());
                    std::string valueKey = data.substr(p2 + 1, valueKeyLen);
                    p1 = data.find('!', p2);
                    p2 = p1;
                    p1 = data.find('$', p2);
                    valueLen = atoi(interceptString(data, p2 + 1, p1).c_str());
                    std::string value = data.substr(p1 + 1, valueLen);
                    if (valueSize > 1) {
                        p1 = data.find('!', p2 + 1);
                    }
                    addKey(dbObj::dbHash, key, valueKey, value);
                }
                if (expireTime > Timestamp::now()) {
                    setPExpireTime(dbObj::dbList, key, expireTime);
                }
                p1 += 1 + valueLen;
            } while (data.substr(p1, 2) == "ST");
            continue;
        }
        if (type == dbObj::dbSet) {
            do {
                p2 = data.find('!', p1);
                Timestamp expireTime(atoi(interceptString(data, p1 + 2, p2).c_str()));
                p1 = data.find('#', p2);
                int keyLen = atoi(interceptString(data, p2 + 1, p1).c_str());
                std::string key = data.substr(p1 + 1, keyLen);
                p2 = data.find('!', p1);
                p1 = data.find('!', p2 + 1);
                int valueSize = atoi(interceptString(data, p2 + 1, p1).c_str());
                int valueLen = 0;
                while (valueSize--) {
                    p2 = p1;
                    p1 = data.find('$', p2);
                    valueLen = atoi(interceptString(data, p2 + 1, p1).c_str());
                    std::string value = data.substr(p1 + 1, valueLen);

                    if (valueSize > 1) {
                        p1 = data.find('!', p2 + 1);
                    }
                    addKey(dbObj::dbSet, key, value, dbObj::defaultObjValue);
                }
                if (expireTime > Timestamp::now()) {
                    setPExpireTime(dbObj::dbSet, key, expireTime);
                }
                p1 += 1 + valueLen;
            } while (data.substr(p1, 2) == "ST");
            continue;
        }
        if(type == dbObj::dbZSet){
            do{
                p2 = data.find('!',p1);
                Timestamp expireTime(atoi(interceptString(data, p1 + 2, p2).c_str()));
                p1 = data.find('#',p2);
                int keyLen = atoi(interceptString(data,p2 + 1,p1).c_str());
                std::string key = data.substr(p1 + 1,keyLen);
                p2 = data.find('!',p1);
                p1 = data.find('!',p2 + 1);
                int valueSize = atoi(interceptString(data,p2 + 1,p1).c_str());
                int valueLen = 0;
                while(valueSize--){
                    p2 = data.find('#',p1);
                    int valueKeyLen = atoi(interceptString(data,p1 + 1,p2).c_str());
                    std::string valueKey = data.substr(p2 + 1,valueKeyLen);
                    p1 = data.find('!',p2);
                    p2 = p1;
                    p1 = data.find('$',p2);
                    valueLen = atoi(interceptString(data,p2 + 1,p1).c_str());
                    std::string value = data.substr(p1 + 1,valueLen);
                    if(valueSize > 1)
                        p1 = data.find('!',p2 + 1);
                    addKey(dbObj::dbZSet,key,valueKey,value);
                }
                p1 += 1 + valueLen;
            } while (data.substr(p1,2) == "ST");
            continue;
        }
    }
}

bool Database::addKey(const int type, const std::string &key, const std::string &objKey,
                      const std::string &objValue) {
    // 若为字符串类型
    if (type == dbObj::dbString) {
        auto it = String_.find(key);
        if (it == String_.end()) {
            String_.insert(std::make_pair(key, objKey));
        } else {
            String_[key] = objKey;
        }
    } else if (type == dbObj::dbList) {
        auto it = List_.find(key);
        if (it == List_.end()) {
            std::list<std::string, __gnu_cxx::__pool_alloc<std::string>> tmp;
            tmp.emplace_back(objKey);
            List_.insert(std::make_pair(key, tmp));
        } else {
            it->second.emplace_back(objKey);
        }
    } else if (type == dbObj::dbHash) {
        auto it = Hash_.find(key);
        if (it == Hash_.end()) {
            std::map<std::string, std::string, std::less<>, __gnu_cxx::__pool_alloc<std::pair<const std::string, std::string>>> tmp;
            tmp.insert(std::make_pair(objKey, objValue));
            Hash_.insert(std::make_pair(key, tmp));
        } else {
            it->second[objKey] = objValue;
        }
    } else if (type == dbObj::dbSet) {
        auto it = Set_.find(key);
        if (it == Set_.end()) {
            std::unordered_set<std::string, std::hash<std::string>, std::equal_to<>, __gnu_cxx::__pool_alloc<std::string>> tmp;
            tmp.insert(objKey);
            Set_.insert(std::make_pair(key, tmp));
        } else {
            it->second.insert(objKey);
        }
    } else if (type == dbObj::dbZSet) {
        auto it = ZSet_.find(key);
        if (it == ZSet_.end()) {
            SP_SkipList skipList(new SkipList());
            skipList->insertNode(objKey, atoi(objValue.c_str()));

            ZSet_.insert(std::make_pair(key, skipList));
        } else {
            auto iter = ZSet_.find(key);
            iter->second->insertNode(objKey, atoi(objValue.c_str()));
        }
    } else {
        std::cout << "Unknown type" << std::endl;
        return false;
    }
    std::cout << "Add key successfully" << std::endl;
    return true;
}

bool Database::delKey(const int type, const std::string &key) {
    if (type == dbObj::dbString) {
        auto it = String_.find(key);
        if (it != String_.end()) {
            String_.erase(key);
            StringExpire_.erase(key);
        } else {
//            std::cout<<"Not Found"<<std::endl;
            return false;
        }
    } else if (type == dbObj::dbList) {
        auto it = List_.find(key);
        if (it != List_.end()) {
            List_.erase(key);
            ListExpire_.erase(key);
        } else {
            return false;
        }
    } else if (type == dbObj::dbHash) {
        auto it = Hash_.find(key);
        if (it != Hash_.end()) {
            Hash_.erase(key);
            HashExpire_.erase(key);
        } else {
            return false;
        }
    } else if (type == dbObj::dbSet) {
        auto it = Set_.find(key);
        if (it != Set_.end()) {
            Set_.erase(key);
            SetExpire_.erase(key);
        } else {
            return false;
        }
    }
    return true;
}

std::string Database::getKey(const int type, const std::string &key) {
    std::string res;

    if (!judgeKeyExpiredTime(type, key)) {
        if (type == dbObj::dbString) {
            auto it = String_.find(key);
            if (it == String_.end()) {
                res = dbStatus::notFound("key").toString();
            } else {
                res = it->second;
            }
        } else if (type == dbObj::dbHash) {
            auto it = Hash_.find(key);
            if (it == Hash_.end()) {
                res = dbStatus::notFound("key").toString();
            } else {
                std::map<std::string, std::string>::iterator iter;
                for (iter = it->second.begin(); iter != it->second.end(); iter++) {
                    res += iter->first + ':' + iter->second + ' ';
                }
            }
        } else if (type == dbObj::dbSet) {
            auto it = Set_.find(key);
            if (it == Set_.end()) {
                res = dbStatus::notFound("key").toString();
            } else {
                for (const auto &iter: it->second) {
                    res += iter + ' ';
                }
            }
        }else if(type == dbObj::dbZSet){    // ZSet中的key,可能包含range范围,格式为 key:low@high 或 key
            double low = -DBL_MAX;
            double high = DBL_MAX;
            std::string curKey = key;

            if(key.find(':') != std::string::npos){
                int p1 = key.find(':');
                int p2 = key.find('@');
                curKey = key.substr(0,p1);
                low = std::stod(key.substr(p1+1,p2-p1-1));
                high = std::stod(key.substr(p2+1,key.size() - p2));
            }
            auto it = ZSet_.find(curKey);
            if (it == ZSet_.end()) {
                res = dbStatus::notFound("key").toString();
            }else {
                rangespec range(low, high);
                std::vector<skiplistNode *> nodes(it->second->getNodeInRange(range));
                for (auto node: nodes) {
                    res += node->obj_ + ':' + std::to_string(node->score_) + '\n';
                }
                res.pop_back();
            }
        }
    } else {
        delKey(type, key);
        res = "The key has expired and will be deleted";
    }

    return res;
}

bool Database::setPExpireTime(const int type, const std::string &key, double expiredTime /* milliSeconds*/) {
    if (type == dbObj::dbString) {
        auto it = String_.find(key);
        if (it != String_.end()) {
            auto now = addTime(Timestamp::now(), expiredTime / Timestamp::kMilliSecondsPerSecond);
            StringExpire_[key] = now;
            return true;
        }
    } else if (type == dbObj::dbList) {
        auto it = List_.find(key);
        if (it != List_.end()) {
            auto now = addTime(Timestamp::now(), expiredTime / Timestamp::kMilliSecondsPerSecond);
            ListExpire_[key] = now;
            return true;
        }
    } else if (type == dbObj::dbHash) {
        auto it = Hash_.find(key);
        if (it != Hash_.end()) {
            auto now = addTime(Timestamp::now(), expiredTime / Timestamp::kMilliSecondsPerSecond);
            HashExpire_[key] = now;
            return true;
        }
    } else if (type == dbObj::dbSet) {
        auto it = Set_.find(key);
        if (it != Set_.end()) {
            auto now = addTime(Timestamp::now(), expiredTime / Timestamp::kMilliSecondsPerSecond);
            SetExpire_[key] = now;
            return true;
        }
    }else if(type == dbObj::dbZSet){
        auto it = ZSet_.find(key);
        if(it != ZSet_.end()){
            auto now = addTime(Timestamp::now(),expiredTime / Timestamp::kMilliSecondsPerSecond);
            ZSetExpire_[key] = now;
            return true;
        }
    }
    return false;
}

bool Database::setPExpireTime(const int type, const std::string &key, const Timestamp &expiredTime) {
    double expired_time = expiredTime.microSecondsSinceEpoch() / Timestamp::kMicroSecondsPerMilliSecond;
    return setPExpireTime(type, key, expired_time);
}

Timestamp Database::getKeyExpiredTime(const int type, const std::string &key) {
    Timestamp tmp;
    if (type == dbObj::dbString) {
        auto it = StringExpire_.find(key);
        if (it != StringExpire_.end()) {
            tmp = it->second;
        } else {
            tmp = Timestamp::invalid();
        }
    } else if (type == dbObj::dbList) {
        auto it = ListExpire_.find(key);
        if (it != ListExpire_.end()) {
            tmp = it->second;
        } else {
            tmp = Timestamp::invalid();
        }
    } else if (type == dbObj::dbHash) {
        auto it = HashExpire_.find(key);
        if (it != HashExpire_.end()) {
            tmp = it->second;
        } else {
            tmp = Timestamp::invalid();
        }
    } else if (type == dbObj::dbSet) {
        auto it = SetExpire_.find(key);
        if (it != SetExpire_.end()) {
            tmp = it->second;
        } else {
            tmp = Timestamp::invalid();
        }
    }else if (type == dbObj::dbZSet){
        auto it = ZSetExpire_.find(key);
        if(it != ZSetExpire_.end()){
            tmp = it->second;
        }else{
            tmp = Timestamp::invalid();
        }
    }

    return tmp;
}

bool Database::judgeKeyExpiredTime(const int type, const std::string &key) {
    Timestamp expired = getKeyExpiredTime(type, key);
    if (expired == Timestamp::invalid()) {
        return false;
    } else {
        Timestamp now = Timestamp::now();
        if (now > expired) {
            return true;
        } else {
            return false;
        }
    }
}

const std::string Database::rpopList(const std::string &key) {
    auto iter = List_.find(key);
    if (iter != List_.end()) {
        if (iter->second.empty()) {
            return dbStatus::notFound("nil").toString();
        }
        std::string res = iter->second.back();
        iter->second.pop_back();
        return res;
    } else {
        return dbStatus::notFound("key").toString();
    }
}

std::string Database::interceptString(const std::string &ss, int p1, int p2) {
    if (p1 > p2) {
        std::swap(p1, p2);
    }
    return ss.substr(p1, p2 - p1);
}