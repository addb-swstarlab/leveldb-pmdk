// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "leveldb/iterator.h"

namespace leveldb {

Iterator::Iterator() {
  cleanup_head_.function = nullptr;
  cleanup_head_.next = nullptr;
  // JH
  use_skiplist_cache = true;
}

Iterator::~Iterator() {
  // JH
  // if (!use_skiplist_cache) {
  //   if (!cleanup_head_.IsEmpty()) {
  //     // printf("RunCleanUpFunc11\n");
  //     cleanup_head_.Run();
  //     // printf("RunCleanUpFunc22\n");
  //     for (CleanupNode* node = cleanup_head_.next; node != nullptr; ) {
  //     // printf("RunCleanUpFunc33\n");
  //       node->Run();
  //     // printf("RunCleanUpFunc44\n");
  //       CleanupNode* next_node = node->next;
  //       delete node;
  //       node = next_node;
  //     }
  //   }
  // }
  if (!cleanup_head_.IsEmpty()) {
    cleanup_head_.Run();
    for (CleanupNode* node = cleanup_head_.next; node != nullptr; ) {
      node->Run();
      CleanupNode* next_node = node->next;
      delete node;
      node = next_node;
    }
  }
}
// JH
// Copy from destructor
// NOTE: Run cleanup function but calling this function, not descriptor.
void Iterator::RunCleanupFunc() {
  if (!cleanup_head_.IsEmpty()) {
    // printf("RunCleanUpFunc1\n");
    cleanup_head_.Run();
    // printf("RunCleanUpFunc2\n");
    for (CleanupNode* node = cleanup_head_.next; node != nullptr; ) {
    // printf("RunCleanUpFunc3\n");
      node->Run();
    // printf("RunCleanUpFunc4\n");
      CleanupNode* next_node = node->next;
      delete node;
      node = next_node;
    }
  }
}

void Iterator::RegisterCleanup(CleanupFunction func, void* arg1, void* arg2) {
  assert(func != nullptr);
  CleanupNode* node;
  if (cleanup_head_.IsEmpty()) {
    node = &cleanup_head_;
  } else {
    node = new CleanupNode();
    node->next = cleanup_head_.next;
    cleanup_head_.next = node;
  }
  node->function = func;
  node->arg1 = arg1;
  node->arg2 = arg2;
}

namespace {

class EmptyIterator : public Iterator {
 public:
  EmptyIterator(const Status& s) : status_(s) { }
  ~EmptyIterator() override = default;

  bool Valid() const override { return false; }
  void Seek(const Slice& target) override { }
  void SeekToFirst() override { }
  void SeekToLast() override { }
  void Next() override { assert(false); }
  void Prev() override { assert(false); }
  Slice key() const override { assert(false); return Slice(); }
  Slice value() const override { assert(false); return Slice(); }
  Status status() const override { return status_; }

 private:
  Status status_;
};

}  // anonymous namespace

Iterator* NewEmptyIterator() {
  return new EmptyIterator(Status::OK());
}

Iterator* NewErrorIterator(const Status& status) {
  return new EmptyIterator(status);
}

}  // namespace leveldb
