//
// Created by wonder on 2021/5/5.
//

#pragma once

#include <iostream>
#include <string>

class Timestamp {
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);

    static Timestamp now();
    static Timestamp invalid();

    std::string toString(bool showMicroSeconds = false) const;

    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

    bool valid() const{ return microSecondsSinceEpoch_ > 0;}
    void swap(Timestamp & that){
        std::swap(microSecondsSinceEpoch_,that.microSecondsSinceEpoch_);
    }

    static const int kMicroSecondsPerSecond = 1000 * 1000;
    static const int kMicroSecondsPerMilliSecond = 1000;
private:
    int64_t microSecondsSinceEpoch_;
};

inline bool operator<(Timestamp lhs,Timestamp rhs){
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}
inline bool operator<=(Timestamp lhs,Timestamp rhs){
    return lhs.microSecondsSinceEpoch() <= rhs.microSecondsSinceEpoch();
}
inline bool operator>(Timestamp lhs,Timestamp rhs){
    return lhs.microSecondsSinceEpoch() > rhs.microSecondsSinceEpoch();
}
inline bool operator>=(Timestamp lhs,Timestamp rhs){
    return lhs.microSecondsSinceEpoch() >= rhs.microSecondsSinceEpoch();
}
inline bool operator==(Timestamp lhs,Timestamp rhs){
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}
inline Timestamp operator+(Timestamp lhs,Timestamp rhs){
    return Timestamp(lhs.microSecondsSinceEpoch() + rhs.microSecondsSinceEpoch());
}
inline Timestamp addTime(Timestamp timestamp,double seconds){
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}
