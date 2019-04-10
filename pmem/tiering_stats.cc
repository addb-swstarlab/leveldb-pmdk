
#include "pmem/tiering_stats.h"
#include <iostream>


namespace leveldb {

  bool Tiering_stats::IsInFileSet(uint64_t file_number) {
    // printf("IsInFileSet\n");
    std::set<uint64_t>::iterator iter = file_set.find(file_number);
    if (iter != file_set.end()) {
      return true;
    }
    return false;
  }

  bool Tiering_stats::IsInSkiplistSet(uint64_t file_number) {
    // printf("IsInSkiplistSet\n");
    std::set<uint64_t>::iterator iter = skiplist_set.find(file_number);
    if (iter != skiplist_set.end()) {
      return true;
    }
    return false;
  }

  void InsertIntoSet(std::set<uint64_t>* set, uint64_t file_number) {
    set->insert(file_number);
  }
  void Tiering_stats::InsertIntoFileSet(uint64_t file_number) {
    // printf("InsertIntoFileSet %d\n", file_number);
    InsertIntoSet(&file_set, file_number);
  }
  void Tiering_stats::InsertIntoSkiplistSet(uint64_t file_number) {
    // printf("InsertIntoSkiplistSet %d\n", file_number);
    InsertIntoSet(&skiplist_set, file_number);
  }
  
  int DeleteFromSet(std::set<uint64_t>* set, uint64_t file_number) {
    return set->erase(file_number);
  }
  int Tiering_stats::DeleteFromFileSet(uint64_t file_number) {
    // printf("DeleteFromFileSet %d\n", file_number);
    return DeleteFromSet(&file_set, file_number);
  }
  int Tiering_stats::DeleteFromSkiplistSet(uint64_t file_number) {
    // printf("DeleteFromSkiplistSet %d\n", file_number);
    return DeleteFromSet(&skiplist_set, file_number);
  }

  void Tiering_stats::PushToFileNumberList(uint64_t file_number) {
    LRU_fileNumber_list.push_back(file_number);
  }
  uint64_t Tiering_stats::PopFromFileNumberList(uint64_t file_number) {
    uint64_t first = LRU_fileNumber_list.front();
    LRU_fileNumber_list.pop_front();
    return first;
  }
  void Tiering_stats::RemoveFromFileNumberList(uint64_t file_number) {
    LRU_fileNumber_list.remove(file_number);
  }
  uint64_t Tiering_stats::GetFileSetSize() {
    return file_set.size();
  }
  uint64_t Tiering_stats::GetSkiplistSetSize() {
    return skiplist_set.size();
  }
} // namespace leveldb