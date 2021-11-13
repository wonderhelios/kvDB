//
// Created by wonder on 2021/11/12.
//

#include <iostream>
#include <thread>
#include "../client/DBClient.h"

const int REQ = 1000;

int main() {
    DBClient client(9981);
    client.Connect();
    // set
    clock_t startTime1 = clock();

    std::thread t1;

    int i = 0;
    while (1) {
        for (i = 0; i < REQ; i++) {
            char buf[10];
            snprintf(buf, 10, "req%d", i);
            std::string t = "set name ";
            t += std::string(buf);
            client.handleRequest(t);
        }
        if (i == REQ) break;
    }
    clock_t endTime1 = clock();

    // get
    clock_t startTime2 = clock();
    while (1) {
        for (i = 0; i < REQ; i++) {
            client.handleRequest("get name");
        }
        if (i == REQ) break;
    }
    clock_t endTime2 = clock();

    std::cout << "set time costs: " << (double) (endTime1 - startTime1) / CLOCKS_PER_SEC << std::endl;
    std::cout << "get time costs: " << (double) (endTime2 - startTime2) / CLOCKS_PER_SEC << std::endl;

    return 0;
}