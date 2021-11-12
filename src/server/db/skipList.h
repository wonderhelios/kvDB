//
// Created by wonder on 2021/11/2.
//

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#define MAX_LEVEL 12

class skiplistLevel;

// class for Node
class skiplistNode {
public:
    skiplistNode() {}

    skiplistNode(const std::string & obj, double score, int level);

    std::unique_ptr<std::unique_ptr<skiplistLevel>[]> levels_;
    std::string obj_;
    double score_;
};

// class for level
class skiplistLevel {
public:
    skiplistLevel()
            : forward_(nullptr){

    }
    skiplistNode *forward_;
};

// class for range
class rangespec {
public:
    rangespec(double min, double max)
            : min_(min), max_(max),
              minex_(true), maxex_(true) {

    }

    double min_, max_;
    bool minex_, maxex_;
};

// class for skip list
class SkipList {
public:
    SkipList();

    ~SkipList();

    // noncopyable
    SkipList(SkipList &) = delete;

    SkipList &operator=(SkipList &) = delete;

    skiplistNode *createNode(const std::string & obj, double score, int level);
    int getRandomLevel();
    void insertNode(const std::string &, double);
    void deleteNode(const std::string &,double);
    unsigned long getCountInRange(rangespec & range);
    std::vector<skiplistNode*> getNodeInRange(rangespec & range);

    unsigned long getLength() { return length_; }

private:
    skiplistNode *header_;
    // 用来保证key不同
    std::unordered_map<std::string,double> keySet_;

    int level_;
    unsigned long length_;

    int valueGteMin(double value,rangespec & spec);
    int valueLteMax(double value,rangespec & spec);
};
