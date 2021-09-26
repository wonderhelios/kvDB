//
// Created by wonder on 2021/9/26.
//

#include "DBServer.h"

int main(){
    EventLoop loop;
    InetAddress localAddr(9981);
    DBServer dbServer(&loop,localAddr);

    dbServer.start();
    loop.loop();

    return 0;
}