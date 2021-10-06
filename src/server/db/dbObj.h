//
// Created by wonder on 2021/9/26.
//

#pragma once
namespace dbObj{

    // 数据类型
    const short dbString = 0;
    const short dbList = 1;

    const std::string defaultObjValue = "NULL";

    //RDB默认保存时间(ms)
    const Timestamp rdbDefaultTime(1000 * Timestamp::kMicroSecondsPerMilliSecond);
}