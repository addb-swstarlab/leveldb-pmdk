/*
 * [2019.03.10][JH]
 * PMDK-based skiplist class
 */
#ifndef PMEM_SKIPLIST_H
#define PMEM_SKIPLIST_H

#include <list>
#include <map>

#include "pmem/layout.h"
#include "pmem/ds/skiplist_buffer.h"
#include "pmem/map/hashmap.h"

// C++
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/pool.hpp>


// use pmem with c++ bindings
namespace pobj = pmem::obj;

namespace leveldb {

  struct skiplist_map_entry;
  struct skiplist_map_node;     // Skiplist Actual node 
  struct root_skiplist;         // Skiplist head
  struct root_skiplist_manager; //  Manager of Multiple Skiplists

  bool file_exists (const std::string &name);
  /* 
   * Dynamic allocation
   * Common function about list, map
   */
  // Free-list
  void PushFreeList(std::list<uint64_t>* free_list, uint64_t index);  // push free-index
  uint64_t PopFreeList(std::list<uint64_t>* free_list);             // return first free-index
  // Allocated-map
  void InsertAllocatedMap(std::map<uint64_t, uint64_t>* allocated_map, 
                          uint64_t file_number, uint64_t index);
  uint64_t GetIndexFromAllocatedMap(std::map<uint64_t, uint64_t>* allocated_map,
                                    uint64_t file_number); // return index
  void EraseAllocatedMap(std::map<uint64_t, uint64_t>* allocated_map, 
                          uint64_t file_number);
  bool CheckMapValidation(std::map<uint64_t, uint64_t>* allocated_map, 
                          uint64_t file_number);
  // Control functions
  uint64_t AddFileAndGetNewIndex(std::list<uint64_t>* free_list,
                                 std::map<uint64_t, uint64_t>* allocated_map,
                                 uint64_t file_number);
  uint64_t GetActualIndex(std::list<uint64_t>* free_list,
            std::map<uint64_t, uint64_t>* allocated_map, uint64_t file_number);

  class PmemSkiplist {
   public:
    PmemSkiplist();
    PmemSkiplist(std::string pool_path);
    ~PmemSkiplist();
    void Init(std::string pool_path);
    void ClearAll();

    /* Wrapper functions */
    void Insert(char* key, char* buffer_ptr, 
                      int key_len, uint64_t file_number);
    void InsertByPtr(void* key_ptr, char* buffer_ptr, int key_len, 
                      uint64_t file_number);
    void InsertNullNode(uint64_t file_number);
    // [Deprecated]
    // char* Get(int index, char *key);
    void Foreach(uint64_t file_number, int (*callback) (
                char* key, char* buffer_ptr, int key_len, void* arg));
    void PrintAll(uint64_t file_number);

    /* Iterator functions */
    PMEMoid* GetPrevOID(uint64_t file_number, char* key);
    PMEMoid* GetOID(uint64_t file_number, char* key);
    PMEMoid* GetFirstOID(uint64_t file_number);    
    PMEMoid* GetLastOID(uint64_t file_number);

    /* Getter */
    PMEMobjpool* GetPool();
    size_t GetFreeListSize();
    size_t GetAllocatedMapSize();

    /* Setter */
    void ResetCurrentNodeToHeader(uint64_t index);

    bool IsFreeListEmpty();

    /* Dynamic allocation*/
    void DeleteFile(uint64_t file_number);

   private:
    struct root_skiplist* root_skiplist_map_;

    /* Actual Skiplist interface */
    TOID(struct skiplist_map_node)* skiplists_;
    TOID(struct skiplist_map_node)* current_node;
    
    /* pmdk access object */
    PMEMobjpool* skiplist_pool_c;
    pobj::pool<root_skiplist_manager> skiplist_pool;
    pobj::persistent_ptr<root_skiplist_manager> root_skiplist_;

    /* Dynamic allocation */
    std::list<uint64_t> free_list_;
    std::map<uint64_t, uint64_t> allocated_map_; // [ file_number -> index ]
  };
  
} // namespace leveldb

#endif