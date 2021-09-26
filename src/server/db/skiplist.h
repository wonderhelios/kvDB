//
// Created by wonder on 2021/9/17.
//

#pragma once

#include <cstring>
#include <fstream>

#define STORE_FILE "dumpFile"
std::string delimiter = ":";

template <typename K, typename V>
class Node {
 public:
  Node(const K k, const V v, int level);
  ~Node();

  K Key() const { return key_; }
  V Value() const { return value_; }
  int Level() const { return level_; }

  void SetValue(V v) { value_ = v; }
  void SetLevel(int level) { level_ = level; }

  // 该节点在各个level索引的下一个数据节点
  Node<K, V> **forward_;

 private:
  K key_;
  V value_;
  int level_{};
};

template <typename K, typename V>
Node<K, V>::Node(const K k, const V v, int level) {
  this->key_ = k;
  this->value_ = v;
  this->level_ = level;

  this->forward_ = new Node<K, V>*[level + 1];
  memset(this->forward_, 0, sizeof(Node<K, V> *) * (level + 1));
}

template <typename K, typename V>
Node<K, V>::~Node<K, V>() {
  delete[] forward_;
}

// SkipList
template <typename K, typename V>
class SkipList {
 public:
  explicit SkipList(int max_level);
  ~SkipList();

  Node<K, V> *CreateNode(const K &, const V &, const int &);
  int InsertElement(const K, const V);
  bool DeleteElement(const K);
  bool SearchElement(const K) const;

  void DisplayList() const;
  void DumpFile();
  void LoadFile();

  int Size() const;

  int GetRandomLevel() const;

 private:
  void GetKeyValueFromString(const std::string &str, std::string *key,
                             std::string *value);
  bool IsValidString(const std::string &str);

 private:
  // 跳表最大支持的层数
  int max_level_;
  // 跳表当前的层数
  int skip_list_level_;
  // 指向头节点的指针
  Node<K, V> *header_;
  // 当前元素个数
  int element_counts_;
  // 文件流
  std::ofstream file_writer_;
  std::ifstream file_reader_;
};

template <typename K, typename V>
SkipList<K, V>::SkipList(int max_level)
  :max_level_(max_level),
  skip_list_level_(0),
  element_counts_(0){
  K k;
  V v;
  this->header_ = new Node<K, V>(k, v, max_level);
}

template <typename K, typename V>
SkipList<K, V>::~SkipList() {
  if (file_reader_ && file_reader_.is_open()) {
    file_reader_.close();
  }
  if (file_writer_ && file_writer_.is_open()) {
    file_writer_.close();
  }
  delete header_;
}

template <typename K, typename V>
Node<K, V> *SkipList<K, V>::CreateNode(const K &key, const V &value,
                                       const int &level) {
  Node<K, V> *node = new Node<K, V>(key, value, level);
  return node;
}

/**
 * 向跳表中插入元素
 * @tparam K
 * @tparam V
 * @param key
 * @param value
 * @return 若元素已经存在,则返回1;若插入成功,则返回0
 */
template <typename K, typename V>
int SkipList<K, V>::InsertElement(const K key, const V value) {

  Node<K, V> *current = this->header_;

  // 用于记录每一级索引中,小于插入值的最大值元素
  Node<K, V> *update[max_level_ + 1];
  memset(update, 0, sizeof(Node<K, V> *) * (max_level_ + 1));

  for (int i = skip_list_level_; i >= 0; --i) {
    while (current->forward_[i] != nullptr &&
           current->forward_[i]->Key() < key) {
      current = current->forward_[i];
    }
    update[i] = current;
  }

  current = current->forward_[0];
  if (current != nullptr && current->Key() == key) {
    std::cout << "key: " << key << ",exists!" << std::endl;
    return 1;
  }

  if (current == nullptr || current->Key() != key) {
    int random_level = GetRandomLevel();
    if (random_level > skip_list_level_) {
      for (int i = skip_list_level_ + 1; i < random_level + 1; i++) {
        update[i] = header_;
      }
      skip_list_level_ = random_level;
    }

    Node<K, V> *inserted_node = CreateNode(key, value, random_level);
    for (int i = 0; i <= random_level; i++) {
      inserted_node->forward_[i] = update[i]->forward_[i];
      update[i]->forward_[i] = inserted_node;
    }
    std::cout << "Successfully inserted key: " << key << ",value: " << value
              << std::endl;
    element_counts_++;
  }
  return 0;
}

template <typename K, typename V>
bool SkipList<K, V>::DeleteElement(const K key) {

  Node<K, V> *current = this->header_;
  Node<K, V> *update[max_level_ + 1];
  memset(update, 0, sizeof(Node<K, V> *) * (max_level_ + 1));

  for (int i = skip_list_level_; i >= 0; --i) {
    while (current->forward_[i] && current->forward_[i]->Key() < key) {
      current = current->forward_[i];
    }
    update[i] = current;
  }
  current = current->forward_[0];

  // 若结点不存在,返回false
  if (current->Key() != key) {
    return false;
  }

  if (current != nullptr && current->Key() == key) {
    for (int i = 0; i <= skip_list_level_; ++i) {
      // 在第i级索引,下一个不等于目标值
      if (update[i]->forward_[i] != current) {
        break;
      }
      update[i]->forward_[i] = current->forward_[i];
    }
  }
  // 释放结点
  delete current;
  while (skip_list_level_ > 0 && !header_->forward_[skip_list_level_]) {
    --skip_list_level_;
  }
  --element_counts_;
  return true;
}

template <typename K, typename V>
bool SkipList<K, V>::SearchElement(const K key) const {
  Node<K, V> *current = header_;

  for (int i = skip_list_level_; i >= 0; --i) {
    while (current->forward_[i] && current->forward_[i]->Key() < key) {
      current = current->forward_[i];
    }
  }
  current = current->forward_[0];
  if (current && current->Key() == key) {
    std::cout << "Found key: " << key << ",value: " << current->Value()
              << std::endl;
    return true;
  }
  std::cout << "Not Found Key: " << key << std::endl;
  return false;
}

template <typename K, typename V>
void SkipList<K, V>::DisplayList() const {
  std::cout << "\n******Skip List******\n";
  for (int i = 0; i <= skip_list_level_; i++) {
    Node<K, V> *node = this->header_->forward_[i];
    std::cout << "Level " << i << ": ";
    while (node != NULL) {
      std::cout << node->Key() << ":" << node->Value() << ";";
      node = node->forward_[i];
    }
    std::cout << std::endl;
  }
}

template <typename K, typename V>
void SkipList<K, V>::DumpFile() {
  std::cout << "****** dump file ******" << std::endl;
  file_writer_.open(STORE_FILE);

  Node<K, V> *node = this->header_->forward_[0];

  while (node != nullptr) {
    file_writer_ << node->Key() << delimiter << node->Value() << "\n";
    std::cout << node->Key() << delimiter << node->Value() << "\n";
    node = node->forward_[0];
  }
  file_writer_.flush();
  file_writer_.close();
}

template <typename K, typename V>
void SkipList<K, V>::LoadFile() {
  std::cout << "****** load file ******" << std::endl;
  file_reader_.open(STORE_FILE);
  std::string line;
  auto *key = new std::string();
  auto *value = new std::string();

  while (getline(file_reader_, line)) {
    GetKeyValueFromString(line, key, value);
    if (key->empty() || value->empty()) {
      continue;
    }
    InsertElement(*key, *value);
    std::cout << "key: " << *key << " value:" << *value << std::endl;
  }
  file_reader_.close();
}

template<typename K,typename V>
int SkipList<K, V>::Size() const {
  return element_counts_;
}

template<typename K,typename V>
void SkipList<K, V>::GetKeyValueFromString(const std::string &str, std::string *key, std::string *value) {
  if(!IsValidString(str)){
    return;
  }
  *key = str.substr(0,str.find(delimiter));
  *value = str.substr(str.find(delimiter) + 1,str.length());
}

template<typename K,typename V>
bool SkipList<K, V>::IsValidString(const std::string &str) {
  if(str.empty()){
    return false;
  }
  if(str.find(delimiter) == std::string::npos){
    return false;
  }
  return true;
}
template<typename K,typename V>
int SkipList<K, V>::GetRandomLevel() const {
  int k = 1;
  while(rand() % 2){
    k++;
  }
  k = (k < max_level_) ? k : max_level_;
  return k;
}