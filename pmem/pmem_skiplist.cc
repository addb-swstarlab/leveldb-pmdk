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
  size_t GetFreeListSize(std::list<uint64_t>* free_list) {
    return free_list->size();
  }
  // Allocated-map
  void InsertAllocatedMap(std::map<uint64_t, uint64_t>* allocated_map, 
                          uint64_t file_number, uint64_t index) {
    allocated_map->emplace(file_number, index);
    if (allocated_map->size() > SKIPLIST_MANAGER_LIST_SIZE) {
      printf("[WARNING][InsertAllocatedMap] map size is full... %d\n", allocated_map->size());
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
    // printf("[AddFileAndGetNewIndex] file_number %d\n", file_number);
    uint64_t new_index = PopFreeList(free_list);
    // printf("[AddFileAndGetNewIndex] freelist size %d\n", free_list->size());
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
      // Get Pool
      skiplist_pool_c = skiplist_pool.get_handle();

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
        // printf("i %d\n",i);
        int res = skiplist_map_create(GetPool(), 
                  &(root_skiplist_map_[i].head), current_node[i], i, &args);
        if (res) printf("[CREATE ERROR %d] %d\n",i ,res);
        else if (i==SKIPLIST_MANAGER_LIST_SIZE-1) printf("[CREATE SUCCESS %d]\n",i);	
        skiplists_[i] = root_skiplist_map_[i].head;
        /* NOTE: Reset current node */
        ResetCurrentNodeToHeader(i);
      }
    } 
    else {
      skiplist_pool = pobj::pool<root_skiplist_manager>::open(pool_path, pool_path);
      skiplist_pool_c = skiplist_pool.get_handle();

      root_skiplist_ = skiplist_pool.get_root();
      root_skiplist_map_ = (struct root_skiplist *)pmemobj_direct_latency(
         root_skiplist_->skiplists.raw() );

  		skiplists_ = (TOID(struct skiplist_map_node) *) malloc(
            sizeof(TOID(struct skiplist_map_node)) * SKIPLIST_MANAGER_LIST_SIZE);
      current_node = (TOID(struct skiplist_map_node) *) malloc(
            sizeof(TOID(struct skiplist_map_node)) * SKIPLIST_MANAGER_LIST_SIZE);
      
      for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
				skiplists_[i] = root_skiplist_map_[i].head;
        ResetCurrentNodeToHeader(i);
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
  void PmemSkiplist::InsertByPtr(char* buffer_ptr,
                                 int key_len, uint64_t file_number) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, 
                                                  file_number);
    int result = skiplist_map_insert_by_ptr(GetPool(), 
                                      skiplists_[actual_index], 
                                      &current_node[actual_index],
                                      buffer_ptr, key_len, actual_index);
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
  // NOTE: [Deprecated] 
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
    return skiplist_pool_c;
  }
  size_t PmemSkiplist::GetFreeListSize() {
    return free_list_.size();
  }
  size_t PmemSkiplist::GetAllocatedMapSize() {
    return allocated_map_.size();
  }
  /* Setter */
  void PmemSkiplist::ResetCurrentNodeToHeader(uint64_t index) {
    current_node[index] = skiplists_[index];
  }


  bool PmemSkiplist::IsFreeListEmpty() {
    return GetFreeListSize() == 0 || 
           GetAllocatedMapSize() >= SKIPLIST_MANAGER_LIST_SIZE;
  }
  // PROGRESS:
  bool PmemSkiplist::IsFreeListEmptyWarning() {
    bool res = GetFreeListSize() < FREE_LIST_WARNING_BOUNDARY; 
    if (res) {
      GarbageCollection(); // Clear untouched garbage in pending_deletion_files
      res = GetFreeListSize() < FREE_LIST_WARNING_BOUNDARY;
    }
    return res;
  }

  /* Dynamic allocation */
  void PmemSkiplist::ResetInfo(uint64_t index, uint64_t file_number) {
    skiplist_map_clear(GetPool(), skiplists_[index]);
    ResetCurrentNodeToHeader(index);
    EraseAllocatedMap(&allocated_map_, file_number); // file_number -> index
    PushFreeList(&free_list_, index);
  }
  void PmemSkiplist::DeleteFile(uint64_t file_number) {
    uint64_t old_index = GetIndexFromAllocatedMap(&allocated_map_, file_number);
    // printf("[DeleteFile] file_number %d index %d\n", file_number, old_index);
    ResetInfo(old_index, file_number);
    // clear1) pending files
    std::set<uint64_t>::iterator set_iter = pending_deletion_files_.find(file_number);
    if (set_iter != pending_deletion_files_.end()) {
      pending_deletion_files_.erase(set_iter);
    } 
    // clear2) referenced
    referenced_files_.remove(file_number);
  }
  // PROGRESS: Check pending_deletion_list (ref_count)
  void PmemSkiplist::DeleteFileWithCheckRef(uint64_t file_number) {
    uint64_t old_index = GetIndexFromAllocatedMap(&allocated_map_, file_number);
    // printf("[DeleteFileWithCheckRef] file_number %d index %d\n", file_number, old_index);
    std::list<uint64_t>::iterator iter = referenced_files_.begin();
    bool seek_in_referenced_list = false;
    for ( ; iter != referenced_files_.end(); iter++) {
      if (*iter == file_number) {
        // printf("[%d %d]\n", *iter, file_number);
        seek_in_referenced_list = true;
        break;
      }
    }
    // NOTE: carefully delete skip list due to synchronization issues.
    if (!seek_in_referenced_list) {
      // printf("DeleteFile imm %d\n", file_number);
      ResetInfo(old_index, file_number);
    } else {
      printf("Insert %d\n", file_number);
      pending_deletion_files_.insert(file_number);
    }
  }

  /* PROGRESS: */
  void PmemSkiplist::Ref(uint64_t file_number) {
    // printf("Ref number %d\n", file_number);
    referenced_files_.push_back(file_number);
  }
  void PmemSkiplist::UnRef(uint64_t file_number) {
    // printf("UnRef number %d\n", file_number);
    uint64_t index = GetIndexFromAllocatedMap(&allocated_map_, file_number);
    std::list<uint64_t>::iterator iter = referenced_files_.begin();
    // First Seek 
    int count = 0;
    for ( ; iter != referenced_files_.end(); iter++) {
      if (*iter == file_number) {
        // printf("[Unref] %d %d\n", *iter, file_number);
        count++;
        referenced_files_.erase(iter);
        break;
      } 
    }
    // DEBUG: special case] UnRef -> Ref
    if (count == 0) {
      printf("[WARN]Unref %d, but count = 0\n", count);
    }
    // Second seek
    std::set<uint64_t>::iterator set_iter = pending_deletion_files_.find(file_number);
    if (set_iter != pending_deletion_files_.end()) {
      bool seek_in_referenced_files = false;
      printf("%d] ", file_number % 10);
      for (iter = referenced_files_.begin(); 
          iter != referenced_files_.end(); 
          iter++) {
        printf("%d ", *iter);
        if (*iter == file_number) {
          printf("pending yet [[%d %d]]\n", *iter, file_number);
          seek_in_referenced_files = true;
          break;
        }
      }
      printf("\n");
      if (!seek_in_referenced_files) {
        pending_deletion_files_.erase(set_iter);
        ResetInfo(index, file_number);
        printf("[SUCCESS] erase pending %d\n", file_number);
      } else {
        printf("[FAILED] pending yet\n");
      }
    }
  }
  // PROGRESS:
  void PmemSkiplist::GarbageCollection() {
    // printf("[GC]\n");
    std::set<uint64_t>::iterator set_iter;
    std::list<uint64_t>::iterator list_iter;
    for ( set_iter = pending_deletion_files_.begin();
          set_iter != pending_deletion_files_.end();
          ) {
      list_iter = std::find(referenced_files_.begin(), 
                            referenced_files_.end(), *set_iter);
      if (list_iter == referenced_files_.end()) {
        // Cannot find = GC candidate
        printf("GC %d\n", *set_iter);
        uint64_t index = GetIndexFromAllocatedMap(&allocated_map_, *set_iter);
        ResetInfo(index, *set_iter);
        set_iter = pending_deletion_files_.erase(set_iter);
      } else {
        // printf("[[%d %d]]\n", *set_iter, *list_iter);
        std::list<uint64_t>::iterator iter = referenced_files_.begin();
        int count = 0;
        for ( ; iter != referenced_files_.end(); iter++) {
            // printf("[[%d %d]]\n", *iter, index);
          if (*iter == *list_iter) {
            count++;
          }
        }
        printf("[[%d %d]] - %d\n", *set_iter, *list_iter, count);
        ++set_iter;
      }
    }
    // printf("[GC End]\n");
  }

  /* Check whether skiplist is valid in a specific version */
  bool PmemSkiplist::CheckNumberIsInPmem(uint64_t file_number) {
    return CheckMapValidation(&allocated_map_, file_number);
  }


} // namespace leveldb 