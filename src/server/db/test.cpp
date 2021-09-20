//
// Created by wonder on 2021/9/19.
//
#include <pthread.h>
#include <chrono>
#include <iostream>
#include "skiplist.h"

#define NUM_THREADS 1
#define TEST_COUNT 1000000

SkipList<int, std::string> skipList(18);

void* insertElement(void* threadId) {
  long tid;
  tid = (long)threadId;
  std::cout << tid << std::endl;
  int tmp = TEST_COUNT / NUM_THREADS;
  for (int i = tid * tmp, count = 0; count < tmp; i++) {
    count++;
    skipList.InsertElement(rand() % TEST_COUNT, "a");
  }
  pthread_exit(NULL);
}

void* getElement(void* threadId) {
  long tid;
  tid = (long)threadId;
  std::cout << tid << std::endl;
  int tmp = TEST_COUNT / NUM_THREADS;
  for (int i = tid * tmp, count = 0; count < tmp; i++) {
    count++;
    skipList.SearchElement(rand() % TEST_COUNT);
  }
  pthread_exit(NULL);
}

int main() {
  srand(time(NULL));
  {
    pthread_t threads[NUM_THREADS];
    int rc;
    int i;
    auto start = std::chrono::high_resolution_clock::now();

    for (i = 0; i < NUM_THREADS; i++) {
      std::cout << "main(): creating threads," << i << std::endl;
      rc = pthread_create(&threads[i], NULL, insertElement, (void*)i);

      if (rc) {
        std::cout << "Error: unable to create thread," << rc << std::endl;
        exit(-1);
      }
    }
    void* ret;
    for (i = 0; i < NUM_THREADS; i++) {
      if (pthread_join(threads[i], &ret) != 0) {
        perror("pthread_create() error");
        exit(3);
      }
    }
    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << "Insert elapsed: " << elapsed.count() << std::endl;
  }
  pthread_exit(NULL);
  return 0;
}