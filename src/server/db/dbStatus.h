//
// Created by wonder on 2021/9/26.
//

#pragma once

class dbStatus {
public:
    dbStatus() : dbState_(resCode::kOK), msg_("") {}
    dbStatus(int dbState, const std::string msg)
            : dbState_(dbState),
              msg_(msg) {
    }
    ~dbStatus() {}

    static dbStatus Ok(){
        return dbStatus();
    }
    static dbStatus notFound(const std::string & msg){
        return dbStatus(kNotFound,msg);
    }
    static dbStatus IOError(const std::string & msg){
        return dbStatus(kIOError,msg);
    }

    std::string toString(){
        if(msg_ == ""){
            return "+OK";
        }else{
            const char * type;
            switch (dbState_) {
                case kOK:
                    type = "+OK";
                    break;
                case kNotFound:
                    type = "-NotFound";
                    break;
                case kIOError:
                    type = "-IO Error: ";
                    break;
                default:
                    break;
            }
            std::string res(type);
            res.append(msg_);
            return res;
        }
    }
private:
    enum resCode {
        kOK = 0, kNotFound, kIOError
    };
    int dbState_;
    std::string msg_;
};
