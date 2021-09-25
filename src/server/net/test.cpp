//
// Created by wonder on 2021/9/20.
//

#include "Server.h"
#include "InetAddress.h"

int main(){
    EventLoop loop;
    InetAddress localAddr(12345);
    Server server(&loop,localAddr);

    server.start();

    loop.loop();
    return 0;
}