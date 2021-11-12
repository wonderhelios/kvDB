//
// Created by wonder on 2021/11/2.
//

#include "skipList.h"

#include <memory>
#include <cassert>

skiplistNode::skiplistNode(const std::string &obj, double score, int level)
        : obj_(obj), score_(score) {

    levels_ = std::unique_ptr<std::unique_ptr<skiplistLevel>[]>(new std::unique_ptr<skiplistLevel>[level]);
    for (int i = 0; i < level; i++) {
        levels_[i] = std::make_unique<skiplistLevel>();
    }
}

// implement for skiplist

SkipList::SkipList()
        : header_(new skiplistNode("", 0, MAX_LEVEL)),
          length_(0),
          level_(1) {

}

SkipList::~SkipList() {
    delete header_;
    header_ = nullptr;
}

skiplistNode *SkipList::createNode(const std::string &obj, double score, int level) {
    auto *node = new skiplistNode(obj, score, level);
    return node;
}

int SkipList::getRandomLevel() {
    // 0.25的概率
    static const unsigned int kBranching = 4;
    int level = 1;
    while (level < MAX_LEVEL && ((rand() % kBranching) == 0)) {
        level++;
    }
    assert(level > 0);
    assert(level <= MAX_LEVEL);

    return level;
}

void SkipList::insertNode(const std::string &obj, double score) {
    // 判断obj是否已经存在了,若已经存在,则删除
    if(keySet_.find(obj) != keySet_.end()){
        deleteNode(obj,keySet_[obj]);
    }
    keySet_[obj] = score;

    // 待插入节点的前驱节点
    skiplistNode *update[MAX_LEVEL];
    skiplistNode *tmp = header_;

    // 从最上层开始遍历
    for (int i = level_ - 1; i >= 0; --i) {
        while (tmp->levels_[i]->forward_ && (tmp->levels_[i]->forward_->score_ < score ||
                                             (tmp->levels_[i]->forward_->score_ == score &&
                                              tmp->levels_[i]->forward_->obj_ != obj))) {
            tmp = tmp->levels_[i]->forward_;
        }
        // 记录第i层遍历到的最后一个节点,即在该节点后插入新节点
        update[i] = tmp;
    }

    // 获得随机的层数,将>=原来level层以上的rank[],update[]更新
    int level = getRandomLevel();
    if (level > level_) {
        for (int i = level_; i < level; i++) {
            update[i] = header_;
        }
        level_ = level;
    }

    // 插入新节点
    tmp = createNode(obj, score, level_);
    for (int i = 0; i < level; i++) {
        tmp->levels_[i]->forward_ = update[i]->levels_[i]->forward_;
        update[i]->levels_[i]->forward_ = tmp;
    }

    // 长度加一
    length_++;
}

void SkipList::deleteNode(const std::string &obj, double score) {
    skiplistNode *update[MAX_LEVEL];
    skiplistNode *tmp = header_;

    for (int i = level_ - 1; i >= 0; i--) {
        while (tmp->levels_[i]->forward_ && (tmp->levels_[i]->forward_->score_ < score ||
                                             (tmp->levels_[i]->forward_->score_ == score &&
                                              tmp->levels_[i]->forward_->obj_ != obj))) {
            tmp = tmp->levels_[i]->forward_;
        }
        update[i] = tmp;
    }

    tmp = tmp->levels_[0]->forward_;
    if (tmp && tmp->score_ == score && tmp->obj_ == obj) {
        // 设置前进指针
        for (int i = 0; i < level_; i++) {
            if (update[i]->levels_[i]->forward_ == tmp) {
                update[i]->levels_[i]->forward_ = tmp->levels_[i]->forward_;
            }
        }

        while (level_ > 1 && header_->levels_[level_ - 1]->forward_ == nullptr) {
            level_--;
        }
        length_--;
    }
    delete tmp;
}

unsigned long SkipList::getCountInRange(rangespec &range) {
    skiplistNode *tmp = header_;
    unsigned long count = 0;

    for (int i = level_ - 1; i >= 0; i--) {
        while (tmp->levels_[i]->forward_ && (range.minex_ ? tmp->levels_[i]->forward_->score_ <= range.min_ :
                                             tmp->levels_[i]->forward_->score_ < range.min_))
            tmp = tmp->levels_[i]->forward_;
    }
    tmp = tmp->levels_[0]->forward_;

    while (tmp && (range.maxex_ ? tmp->score_ < range.max_ :
                   tmp->score_ <= range.max_)) {
        count++;
        tmp = tmp->levels_[0]->forward_;
    }
    return count;
}

std::vector<skiplistNode *> SkipList::getNodeInRange(rangespec &range) {
    skiplistNode *tmp = header_;
    std::vector<skiplistNode *> ret;

    for (int i = level_ - 1; i >= 0; i--) {
        while (tmp->levels_[i]->forward_ && (range.minex_ ? tmp->levels_[i]->forward_->score_ <= range.min_ :
                                             tmp->levels_[i]->forward_->score_ < range.min_))
            tmp = tmp->levels_[i]->forward_;
    }
    tmp = tmp->levels_[0]->forward_;

    while (tmp && (range.maxex_ ? tmp->score_ < range.max_ : tmp->score_ <= range.max_)) {
        ret.emplace_back(tmp);
        tmp = tmp->levels_[0]->forward_;
    }
    return ret;
}

/**
 * 判断给定的value是否大于或者大于等于范围spec中的zmin
 * @param value
 * @param spec
 * @return 0: value小于等于zmin
 *         1: value大于zmin
 */
int SkipList::valueGteMin(double value, rangespec &spec) {
    return spec.minex_ ? (value > spec.min_) : (value >= spec.min_);
}

/**
 * 判断给定的value是否小于或者小于等于范围spec中的zmax
 * @param value
 * @param spec
 * @return 0: value大于等于zmax
 *         1: value小于zmax
 */
int SkipList::valueLteMax(double value, rangespec &spec) {
    return spec.maxex_ ? (value < spec.max_) : (value <= spec.max_);
}
