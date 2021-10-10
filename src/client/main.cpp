//
// Created by wonder on 2021/10/9.
//
#include "DBClient.h"

int main(){
    DBClient client(9981);

    client.Connect();
    client.parseCmd();

    return 0;
}