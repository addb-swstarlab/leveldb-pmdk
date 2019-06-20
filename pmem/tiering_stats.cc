
#include "pmem/tiering_stats.h"
#include <iostream>


namespace leveldb {

  bool Tiering_stats::IsInFileSet(uint64_t number) {
    std::set<uint64_t>::iterator iter = file_set.find(number);
    if (iter != file_set.end()) {
      return true;
    }
    return false;
  }
  bool Tiering_stats::IsInSkiplistSet(uint64_t number) {
    std::set<uint64_t>::iterator iter = skiplist_set.find(number);
    if (iter != skiplist_set.end()) {
      return true;
    }
    return false;
  }

  void InsertIntoSet(std::set<uint64_t>* set, uint64_t number) {
    std::pair<std::set<uint64_t>::iterator, bool> pair = set->insert(number);
    if (!(pair.second == true)) {
      printf("[WARN][InsertIntoSet] cannot insert into set\n");
    }
  }
  void Tiering_stats::InsertIntoFileSet(uint64_t number) {
    InsertIntoSet(&file_set, number);
  }
  void Tiering_stats::InsertIntoSkiplistSet(uint64_t number) {
    InsertIntoSet(&skiplist_set, number);
  }
  
  int DeleteFromSet(std::set<uint64_t>* set, uint64_t number) {
    return set->erase(number);
  }
  void Tiering_stats::DeleteFromFileSet(uint64_t number) {
    if (DeleteFromSet(&file_set, number) <= 0) {
      printf("[WARN][DeleteFromFileSet] no deleted_file %d in file set\n", number);
    }
  }
  void Tiering_stats::DeleteFromSkiplistSet(uint64_t number) {
    if (DeleteFromSet(&skiplist_set, number) <= 0) {
      // printf("[WARN][DeleteFromSkiplistSet] no deleted_file %d in skiplist set\n", number);
      DeleteFromFileSet(number);
    }
  }

  void Tiering_stats::PushToNumberListInPmem(int level, uint64_t number) {
    level_number ln;
    ln.level = level;
    ln.number = number;
    LRU_fileNumber_list[number % NUM_OF_SKIPLIST_MANAGER].push_back(ln);
  }
  /* Deprecated function */
  // level_number Tiering_stats::PopFromNumberListInPmem(uint64_t number) {
  //   int index = number % NUM_OF_SKIPLIST_MANAGER;
  //   if (LRU_fileNumber_list[index].size() == 0) {
  //     printf("[WARNING][Tiering_stats][PopFromNumberListInPmem] List is empty..\n");
  //   }
  //   level_number first = LRU_fileNumber_list[index].front();
  //   LRU_fileNumber_list[index].pop_front();
  //   return first;
  // }
  void Tiering_stats::RemoveFromNumberListInPmem(uint64_t number) {
    int index = number % NUM_OF_SKIPLIST_MANAGER;
    std::list<level_number>::iterator iter = LRU_fileNumber_list[index].begin();
    while ( iter != LRU_fileNumber_list[index].end()) {
      if (iter->number == number) {
          LRU_fileNumber_list[index].erase(iter);
        break;
      }
      iter++;
    }
  }
  level_number Tiering_stats::GetElementFromNumberListInPmem(uint64_t number, uint64_t n) {
    int index = number % NUM_OF_SKIPLIST_MANAGER;
    // printf("%d] size %d \n",n, LRU_fileNumber_list[index].size());
    std::list<level_number>::iterator iter = 
            LRU_fileNumber_list[index].begin();
    uint64_t res = 0;
    if (LRU_fileNumber_list[index].size() > n) {
      for (int i=0; i<n; i++) {
        iter++;
      }
      // res = *iter;
      return *iter;
    } else {
      printf("[WARNING][GetElementFromNumberListInPmem]%d] n %d > size %d ..\n", number, n, LRU_fileNumber_list[index].size());
      return LRU_fileNumber_list[index].front();
    }
  }


  uint64_t Tiering_stats::GetFileSetSize() {
    return file_set.size();
  }
  uint64_t Tiering_stats::GetSkiplistSetSize() {
    return skiplist_set.size();
  }
} // namespace leveldb