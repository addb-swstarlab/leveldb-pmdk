/*
 * [2019.03.21][JH]
 * PMDK-based hashmap class
 * Need to optimize some functions...
 */

#ifndef PMEM_HASHMAP_H
#define PMEM_HASHMAP_H

// #include <list>
// #include <map>

// #include "pmem/layout.h"
#include "pmem/map/hashmap.h"

// C++
// #include <libpmemobj++/persistent_ptr.hpp>
// #include <libpmemobj++/make_persistent_array.hpp>
// #include <libpmemobj++/make_persistent.hpp>
// #include <libpmemobj++/transaction.hpp>
// #include <libpmemobj++/pool.hpp>

// TEST:
#include "pmem/ds/hashmap_atomic.h"
#include "pmem/pmem_skiplist.h"

// use pmem with c++ bindings
namespace pobj = pmem::obj;

namespace leveldb {

  // TEST: hashmap
  struct entry;
  struct entry_args;
  struct buckets;
  struct hashmap_atomic;
  struct root_hashmap;
  struct root_hashmap_manager;


  /* TEST: PMDK-based hashmap class */
  class PmemHashmap {
   public:
    PmemHashmap();
    PmemHashmap(std::string pool_path);
    ~PmemHashmap();
    void Init(std::string pool_path);
    
    /* Wrapper functions */
    void Insert(char* key, char* buffer_ptr, 
                      int key_len, uint64_t file_number);
    void InsertByPtr(void* key_ptr, char* buffer_ptr, int key_len, 
                      uint64_t file_number);
    // void InsertNullNode(uint64_t file_number);
    // // char* Get(int index, char *key);
    void Foreach(uint64_t file_number, int (*callback) (
          char* key, char* buffer_ptr, void* key_ptr, int key_len, void* arg));
    void PrintAll(uint64_t file_number);
    void ClearAll();

    // /* Iterator functions */
    PMEMoid* GetPrevOID(uint64_t file_number, TOID(struct entry) current_entry);
    PMEMoid* GetNextOID(uint64_t file_number, TOID(struct entry) current_entry);
    PMEMoid* GetFirstOID(uint64_t file_number);    
    PMEMoid* GetLastOID(uint64_t file_number);
    PMEMoid* SeekOID(uint64_t file_number, char* key, int key_len);

    /* Getter */
    PMEMobjpool* GetPool();

   private:
    struct root_hashmap* root_hashmap_;

    /* Actual Skiplist interface */
    TOID(struct hashmap_atomic)* hashmap_;

    TOID(struct entry)* current_entry;
    
    pobj::pool<root_hashmap_manager> hashmap_pool;
    pobj::persistent_ptr<root_hashmap_manager> root_hashmap_ptr_;

    /* Dynamic allocation */
    std::list<uint64_t> free_list_;
    std::map<uint64_t, uint64_t> allocated_map_; // [ file_number -> index ]
  };

} // namespace leveldb

#endif