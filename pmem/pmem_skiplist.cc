/*
 * [2019.03.10][JH]
 * PMDK-based skiplist class
 * Include dynamic allocation methods and for iterator functions
 */

#include <iostream>
#include <fstream>
#include "pmem/pmem_skiplist.h"

namespace leveldb {
  /* Structure for skiplist */
  struct skiplist_map_entry {
    char* buffer_ptr;
  };
  struct skiplist_map_node {
    TOID(struct skiplist_map_node) next[SKIPLIST_LEVELS_NUM];
    struct skiplist_map_entry entry;
  };
  // Register root-manager & node structrue
  // Skiplist single-node
  struct root_skiplist { // head node
    TOID(struct skiplist_map_node) head;
  };
  // Skiplists manager
  struct root_skiplist_manager {
    pobj::persistent_ptr<root_skiplist[]> skiplists;
  };

  bool file_exists (const std::string &name) {
    std::ifstream f (name.c_str ());
    return f.good ();
  }
  static int
  Print_skiplist(char* key, char* value, int key_len, void *arg)
  {
    // FIXME: Get value from value_ptr
    if (strcmp(key, "") != 0)
      printf("[print] [key %d]:'%s', [value]:'%s'\n", key_len, key, value);
    delete[] key;
    return 0;
  }
  /*
   * NOTE: Common function about list, map
   *       be used in PmemSkiplist, PmemBuffer
   */
  // Free-list
  void PushFreeList(std::list<uint64_t>* free_list, uint64_t index) {
    free_list->push_back(index);
  } 
  uint64_t PopFreeList(std::list<uint64_t>* free_list) {
    // return free_list_
    if (free_list->size() == 0) { 
      printf("[ERROR] free_list is empty... :( \n");
      abort();
    }
    uint16_t res = free_list->front();
    free_list->pop_front();
    return res;
  }
  // Allocated-map
  void InsertAllocatedMap(std::map<uint64_t, uint64_t>* allocated_map, 
                          uint64_t file_number, uint64_t index) {
    allocated_map->emplace(file_number, index);
    if (allocated_map->size() >= SKIPLIST_MANAGER_LIST_SIZE) {
      printf("[WARNING][InsertAllocatedMap] map size is full...\n");
    }
  }
  uint64_t GetIndexFromAllocatedMap(std::map<uint64_t, uint64_t>* allocated_map,
                                    uint64_t file_number) {
    std::map<uint64_t, uint64_t>::iterator iter = allocated_map->find(file_number);
    if (iter == allocated_map->end()) {
      printf("[WARNING][GetAllocatedMap] Cannot get %d from allocated map\n", file_number);
    }
    return iter->second;
  }
  void EraseAllocatedMap(std::map<uint64_t, uint64_t>* allocated_map, 
                          uint64_t file_number) {
    int res = allocated_map->erase(file_number);
    if (!res) {
      printf("[WARNING][EraseAllocatedMap] fail to erase %d\n", file_number);
    }
  }
  bool CheckMapValidation(std::map<uint64_t, uint64_t>* allocated_map, 
                          uint64_t file_number) {
    std::map<uint64_t, uint64_t>::iterator iter = allocated_map->find(file_number);
    if (iter == allocated_map->end()) {
      return false;
    }
    return true;
  }
  // Control functions
  uint64_t AddFileAndGetNewIndex(std::list<uint64_t>* free_list,
                                 std::map<uint64_t, uint64_t>* allocated_map,
                                 uint64_t file_number) {
    uint64_t new_index = PopFreeList(free_list);
    InsertAllocatedMap(allocated_map, file_number, new_index);
    return new_index;
  }
  uint64_t GetActualIndex(std::list<uint64_t>* free_list,
            std::map<uint64_t, uint64_t>* allocated_map, uint64_t file_number) {
    return CheckMapValidation(allocated_map, file_number) ? 
            GetIndexFromAllocatedMap(allocated_map, file_number) :
            AddFileAndGetNewIndex(free_list, allocated_map, file_number);
  }

  /* PMDK-based skiplist */
  PmemSkiplist::PmemSkiplist() {
    Init(SKIPLIST_MANAGER_PATH);
  }
  PmemSkiplist::PmemSkiplist(std::string pool_path) {
    Init(pool_path);
  }
  PmemSkiplist::~PmemSkiplist() {
    free(skiplists_);
    free(current_node);
    pmemobj_close(GetPool());
  }
  void PmemSkiplist::Init(std::string pool_path) {
    if(!file_exists(pool_path)) {
      skiplist_pool = pobj::pool<root_skiplist_manager>::create (
                      pool_path, pool_path, 
                      (unsigned long)SKIPLIST_MANAGER_POOL_SIZE, 0666);
      root_skiplist_ = skiplist_pool.get_root();
      pobj::transaction::exec_tx(skiplist_pool, [&] {
        // Allocate multiple skiplists
        root_skiplist_->skiplists = 
              pobj::make_persistent<root_skiplist[]>(SKIPLIST_MANAGER_LIST_SIZE);
      });
      root_skiplist_map_ = (struct root_skiplist *)pmemobj_direct_latency(
         root_skiplist_->skiplists.raw() );

  		skiplists_ = (TOID(struct skiplist_map_node) *) malloc(
            sizeof(TOID(struct skiplist_map_node)) * SKIPLIST_MANAGER_LIST_SIZE);
      current_node = (TOID(struct skiplist_map_node) *) malloc(
            sizeof(TOID(struct skiplist_map_node)) * SKIPLIST_MANAGER_LIST_SIZE);

      struct hashmap_args args; // empty
      /* create */
      for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
        int res = skiplist_map_create(GetPool(), 
                  &(root_skiplist_map_[i].head), current_node[i], i, &args);
        if (res) printf("[CREATE ERROR %d] %d\n",i ,res);
        else if (i==SKIPLIST_MANAGER_LIST_SIZE-1) printf("[CREATE SUCCESS %d]\n",i);	
        skiplists_[i] = root_skiplist_map_[i].head;
        /* NOTE: Reset current node */
        current_node[i] = skiplists_[i];
      }
    } 
    else {
      skiplist_pool = pobj::pool<root_skiplist_manager>::open(pool_path, pool_path);

      root_skiplist_ = skiplist_pool.get_root();
      root_skiplist_map_ = (struct root_skiplist *)pmemobj_direct_latency(
         root_skiplist_->skiplists.raw() );

  		skiplists_ = (TOID(struct skiplist_map_node) *) malloc(
            sizeof(TOID(struct skiplist_map_node)) * SKIPLIST_MANAGER_LIST_SIZE);
      current_node = (TOID(struct skiplist_map_node) *) malloc(
            sizeof(TOID(struct skiplist_map_node)) * SKIPLIST_MANAGER_LIST_SIZE);
      
      for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
				skiplists_[i] = root_skiplist_map_[i].head;
        current_node[i] = skiplists_[i];
      }
    }
  }
  void PmemSkiplist::ClearAll() {
    for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
      skiplist_map_clear(GetPool(), skiplists_[i]);
      // DA: Push all to freelist
      PushFreeList(&free_list_, i);
    }
  }

  /* Wrapper functions */
  void PmemSkiplist::Insert(char* key, char* buffer_ptr, int key_len, 
                            uint64_t file_number) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, 
                                                  file_number);
    int result = skiplist_map_insert(GetPool(), 
                                      skiplists_[actual_index], 
                                      &current_node[actual_index],
                                      key, buffer_ptr,
                                      key_len, actual_index);
    if(result) { 
      fprintf(stderr, "[ERROR] insert %d\n", file_number);  
      abort();
    } 
  }
  void PmemSkiplist::InsertByPtr(void* key_ptr, char* buffer_ptr, 
                                      int key_len, uint64_t file_number) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, 
                                                  file_number);
    int result = skiplist_map_insert_by_ptr(GetPool(), 
                                      skiplists_[actual_index], 
                                      &current_node[actual_index],
                                      key_ptr, buffer_ptr,
                                      key_len, actual_index);
    if(result) { 
      fprintf(stderr, "[ERROR] insert_by_oid %d\n", file_number);  
      abort();
    } 
  }
  void PmemSkiplist::InsertNullNode(uint64_t file_number) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, 
                                                  file_number);
    int result = skiplist_map_insert_null_node(GetPool(),
                                skiplists_[actual_index], 
                                &current_node[actual_index], 
                                actual_index);
    if(result) {
      fprintf(stderr, "[ERROR] insert_null_node %d\n", file_number);  
    }
  }
  // NOTE: Will be deprecated.. 
  // char* PmemSkiplist::Get(int index, char *key) {
  //   return skiplist_map_get(GetPool(), skiplists_[index], key);
  // }
  void PmemSkiplist::Foreach(uint64_t file_number,
        int (*callback)(char* key, char* buffer_ptr, int key_len, void* arg)) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, 
                                                  file_number);
    int res = skiplist_map_foreach(GetPool(), 
                                  skiplists_[actual_index], callback, nullptr);
  }
  void PmemSkiplist::PrintAll(uint64_t file_number) {
    Foreach(file_number, Print_skiplist);
  }
  

  /* Iterator functions */
  PMEMoid* PmemSkiplist::GetPrevOID(uint64_t file_number, char* key) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, 
                                                  file_number);
    return skiplist_map_get_prev_OID(GetPool(), skiplists_[actual_index], key);
  }
  PMEMoid* PmemSkiplist::GetOID(uint64_t file_number, char* key) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, 
                                                  file_number);
    return skiplist_map_get_OID(GetPool(), skiplists_[actual_index], key);
  }
  PMEMoid* PmemSkiplist::GetFirstOID(uint64_t file_number) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, 
                                                  file_number);
    return skiplist_map_get_first_OID(GetPool(), skiplists_[actual_index]);
  }
  PMEMoid* PmemSkiplist::GetLastOID(uint64_t file_number) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, 
                                                  file_number);
    return skiplist_map_get_last_OID(GetPool(), skiplists_[actual_index]);
  }

  /* Getter */
  PMEMobjpool* PmemSkiplist::GetPool() {
    return skiplist_pool.get_handle();
  }
  size_t PmemSkiplist::GetFreeListSize() {
    return free_list_.size();
  }
  size_t PmemSkiplist::GetAllocatedMapSize() {
    return allocated_map_.size();
  }

  /* Dynamic allocation */
  void PmemSkiplist::DeleteFile(uint64_t file_number) {
    uint64_t old_index = GetIndexFromAllocatedMap(&allocated_map_, file_number);
    // Clear buffer_ptr to nullptr
    skiplist_map_clear(GetPool(), skiplists_[old_index]);
    // Reset current_node
    current_node[old_index] = skiplists_[old_index];
    // Map & List
    EraseAllocatedMap(&allocated_map_, file_number);
    PushFreeList(&free_list_, old_index);
  }

} // namespace leveldb 