#ifndef TIERING_STATS_H
#define TIERING_STATS_H

#include <list>
#include <set>
#include <stdint.h>
#include "pmem/layout.h"

/* Tiering trigger options */
// Opt1: Simple level tiering
#define PMEM_SKIPLIST_LEVEL_THRESHOLD 3

// Opt2: Cold data tiering
#define L0_LIFETIME_THRESHOLD 8
#define L1_LIFETIME_THRESHOLD 20
#define L2_LIFETIME_THRESHOLD 70
#define L3_LIFETIME_THRESHOLD 405
#define L4_LIFETIME_THRESHOLD 4340
// L5, L6 -> immediately flush 

// Opt3: LRU tiering


namespace leveldb {

  struct LevelNumberPair {
    int level;
    uint64_t number;
  } typedef level_number;

  struct tiering {
   public:
    bool IsInFileSet(uint64_t number);
    bool IsInSkiplistSet(uint64_t number);
    // void InsertIntoSet(std::set<uint64_t>* set, uint64_t file_number);
    void InsertIntoFileSet(uint64_t number);
    void InsertIntoSkiplistSet(uint64_t number);
    // int DeleteFromSet(std::set<uint64_t>* set, uint64_t file_number);
    void DeleteFromFileSet(uint64_t number);
    void DeleteFromSkiplistSet(uint64_t number);
    
    void PushToNumberListInPmem(int level, uint64_t number);
    void RemoveFromNumberListInPmem(uint64_t number);
    level_number GetElementFromNumberListInPmem(uint64_t number, uint64_t n);
    
    /* Deprecated function */
    // level_number PopFromNumberListInPmem(uint64_t number);

    uint64_t GetFileSetSize();
    uint64_t GetSkiplistSetSize();

   private:
    // Common sets
    std::set<uint64_t> file_set;
    std::set<uint64_t> skiplist_set;
    // ColdDataTiering, LRUTiering 
    std::list<level_number> LRU_fileNumber_list[NUM_OF_SKIPLIST_MANAGER]; // <level, Number> 
  } typedef Tiering_stats;

} // namespace leveldb
#endif