#ifndef TIERING_STATS_H
#define TIERING_STATS_H

#include <list>
#include <set>
#include <stdint.h>

/* Tiering trigger options */
// Opt1: Simple level tiering
#define SIMPLE_LEVEL 3
// Opt2: Cold data tiering
// Opt3: LRU tiering
#define MAX_PMEM_SST 50000


namespace leveldb {

  struct tiering {
   public:
    bool IsInFileSet(uint64_t file_number);
    bool IsInSkiplistSet(uint64_t file_number);
    // void InsertIntoSet(std::set<uint64_t>* set, uint64_t file_number);
    void InsertIntoFileSet(uint64_t file_number);
    void InsertIntoSkiplistSet(uint64_t file_number);
    // int DeleteFromSet(std::set<uint64_t>* set, uint64_t file_number);
    int DeleteFromFileSet(uint64_t file_number);
    int DeleteFromSkiplistSet(uint64_t file_number);
    
    void PushToFileNumberList(uint64_t file_number);
    uint64_t PopFromFileNumberList(uint64_t file_number);
    void RemoveFromFileNumberList(uint64_t file_number);

    uint64_t GetFileSetSize();
    uint64_t GetSkiplistSetSize();

   private:
    // Common sets
    std::set<uint64_t> file_set;
    std::set<uint64_t> skiplist_set;
    // ColdDataTiering, LRUTiering 
    std::list<uint64_t> LRU_fileNumber_list;
  } typedef Tiering_stats;

} // namespace leveldb
#endif