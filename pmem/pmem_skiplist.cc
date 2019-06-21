/*
 * 
 */

#include <fstream>
#include "pmem/pmem_skiplist.h"
#define SKIPLIST_LEVELS_NUM 12 // redefine

namespace leveldb {
  inline bool
  file_exists (const std::string &name)
  {
    std::ifstream f (name.c_str ());
    return f.good ();
  }
  // NOTE: For Iterator
  struct skiplist_map_entry {
    PMEMoid key;
    uint8_t key_len;
    PMEMoid value;
    uint8_t value_len;
  };
  struct skiplist_map_node {
    TOID(struct skiplist_map_node) next[SKIPLIST_LEVELS_NUM];
    struct skiplist_map_entry entry;
  };

  // Register root-manager & node structrue
  // Skiplist single-node
  struct root_skiplist_map {
    TOID(struct map) map;
  };
  // Skiplists manager
  struct root_skiplist_manager {
    int count;
    PMEMoid skiplists;
    // Offsets
  };
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

  PmemSkiplist::PmemSkiplist() {
    Init(SKIPLIST_MANAGER_PATH);
  }
  PmemSkiplist::PmemSkiplist(std::string pool_path) {
    Init(pool_path);
  }
  PmemSkiplist::~PmemSkiplist() {
    printf("destructor\n");
    free(map);
    pmemobj_close(pool);
  }
  void PmemSkiplist::Init(std::string pool_path) {
    ops = MAP_SKIPLIST;
    if(!file_exists(SKIPLIST_MANAGER_PATH)) {
      /* Initialize pool & map_ctx */
      skiplist_pool = pobj::pool<root_skiplist_manager>::create (
                      pool_path, pool_path, 
                      (unsigned long)SKIPLIST_MANAGER_POOL_SIZE, 0666);
      pool = skiplist_pool.get_handle();

      // pool = pmemobj_create(pool_path.c_str(), 
      //             POBJ_LAYOUT_NAME(root_skiplist_manager), 
      //             (unsigned long)SKIPLIST_MANAGER_POOL_SIZE, 0666); 
      if (pool == NULL) {
        printf("[ERROR] pmemobj_create\n");
        abort();
      }
      mapc = map_ctx_init(ops, pool);
      if (!mapc) {
        pmemobj_close(pool);
        printf("[ERROR] map_ctx_init\n");
        abort();
      }		
      struct hashmap_args args; // empty
      /* 
      * 1) Get root and make root ptr.
      *    skiplists' PMEMoid is oid of overall skiplists
      */
      root = POBJ_ROOT(pool, struct root_skiplist_manager);
      struct root_skiplist_manager *root_ptr = D_RW(root);
      /*
      * 2) Initialize root structure
      *    It includes malloc overall skiplists
      */
      root_ptr->count = SKIPLIST_MANAGER_LIST_SIZE;
      // printf("%d\n", root_ptr->count);
      TX_BEGIN(pool) {
        root_ptr->skiplists = pmemobj_tx_zalloc(
                        sizeof(root_skiplist_map) * SKIPLIST_MANAGER_LIST_SIZE, 
                        SKIPLIST_MAP_TYPE_OFFSET + 1000);
      } TX_ONABORT {
        printf("[ERROR] tx_abort on allocating skiplists\n");
        abort();
      } TX_END
      /* 
      * 3) map_create for all indices
      */
      skiplists = (struct root_skiplist_map *)pmemobj_direct(root_ptr->skiplists);
  		map = (TOID(struct map) *) malloc(
                        sizeof(TOID(struct map)) * SKIPLIST_MANAGER_LIST_SIZE);
      current_node = (TOID(struct map) *) malloc(
            sizeof(TOID(struct map)) * SKIPLIST_MANAGER_LIST_SIZE);

      /* create */
      for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
        // int res = map_create(mapc, &(skiplists[i].map), i, &args); 
        int res = map_create(mapc, &(skiplists[i].map), &current_node[i], i, &args); 
        if (res) printf("[CREATE ERROR %d] %d\n",i ,res);
        else if (i==SKIPLIST_MANAGER_LIST_SIZE-1) printf("[CREATE SUCCESS %d]\n",i);	
        map[i] = skiplists[i].map;
        current_node[i] = map[i];
      }
    } else {
      skiplist_pool = pobj::pool<root_skiplist_manager>::open(pool_path, pool_path);
      pool = skiplist_pool.get_handle();

      // pool = pmemobj_open(pool_path.c_str(), 
      //                                 POBJ_LAYOUT_NAME(root_skiplist_manager));
      if (pool == NULL) {
        printf("[ERROR] pmemobj_create\n");
        abort();
		  }
      mapc = map_ctx_init(ops, pool);
      if (!mapc) {
        pmemobj_close(pool);
        printf("[ERROR] map_ctx_init\n");
        abort();
      }
      root = POBJ_ROOT(pool, struct root_skiplist_manager);
      struct root_skiplist_manager *root_ptr = D_RW(root);
      skiplists = (struct root_skiplist_map *)pmemobj_direct(root_ptr->skiplists);
      map = (TOID(struct map) *) malloc(
                        sizeof(TOID(struct map)) * SKIPLIST_MANAGER_LIST_SIZE);
      current_node = (TOID(struct map) *) malloc(
            sizeof(TOID(struct map)) * SKIPLIST_MANAGER_LIST_SIZE);

      for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
				map[i] = skiplists[i].map;
        current_node[i] = map[i];
      }
    }
  }
  
  PMEMobjpool* PmemSkiplist::GetPool() {
    return pool;
  }

  void PmemSkiplist::ClearAll() {
    for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
      // skiplist_map_clear(GetPool(), map[i]);
      map_clear(mapc, map[i]);
      // DA: Push all to freelist
      PushFreeList(&free_list_, i);
    }
  }

  void PmemSkiplist::Insert(char *key, char *value, int key_len, 
                            int value_len, int index) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, index);
    int result = map_insert(mapc, map[actual_index], &current_node[actual_index], 
    // int result = map_insert(mapc, map[actual_index], key, value, key_len, 
                            key, value, key_len, 
                            value_len, actual_index);
    if(result) { 
      fprintf(stderr, "[ERROR] insert %d\n", actual_index);  
      abort();
    } 
    // else if (!res) printf("insert %d] success\n", index);
  }
  // NOTE: Will be deprecated.. 
  char* PmemSkiplist::Get(int index, char *key) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, index);
    return map_get(mapc, map[actual_index], key);
  }

  // SOLVE: Implement Internal GetTOID
  const PMEMoid* PmemSkiplist::GetPrevOID(int index, char *key) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, index);
    return map_get_prev_OID(mapc, map[actual_index], key);
  }
  const PMEMoid* PmemSkiplist::GetNextOID(int index, char *key) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, index);
    return map_get_next_OID(mapc, map[actual_index], key);
  }
  const PMEMoid* PmemSkiplist::GetFirstOID(int index) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, index);
    return map_get_first_OID(mapc, map[actual_index]);
  }
  const PMEMoid* PmemSkiplist::GetLastOID(int index) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, index);
    return map_get_last_OID(mapc, map[actual_index]);
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
    map_clear(mapc, map[old_index]);
    // Reset current_node
    current_node[old_index] = map[old_index];
    // Map & List
    EraseAllocatedMap(&allocated_map_, file_number);
    PushFreeList(&free_list_, old_index);
  }

  /* SOLVE: Pmem-based Iterator */
  PmemIterator::PmemIterator(PmemSkiplist *pmem_skiplist) 
    : index_(0), pmem_skiplist_(pmem_skiplist) {
      // printf("constructor1\n");
  }
  PmemIterator::PmemIterator(int index, PmemSkiplist *pmem_skiplist) 
    : index_(index), pmem_skiplist_(pmem_skiplist) {
      // printf("constructor %d\n", index);
  }
  PmemIterator::~PmemIterator() {
    
  }

  // TODO: Implement index block to reduce unnecessary seek time
  void PmemIterator::SetIndexAndSeek(int index, const Slice& target) {
    index_ = index;
    current_ = const_cast<PMEMoid *>(pmem_skiplist_->GetNextOID(index_, (char *)target.data()));
    if (OID_IS_NULL(*current_)) {
      printf("[ERROR][SetIndexAndSeek] Access Invalid node .. '%s'\n", target.data());
      abort();
    }
  }
  void PmemIterator::Seek(const Slice& target) {
    current_ = const_cast<PMEMoid *>(pmem_skiplist_->GetNextOID(index_, (char *)target.data()));
    if (OID_IS_NULL(*current_)) {
      printf("[ERROR][Seek] Access Invalid node .. '%s'\n", target.data());
      abort();
    }
  }
  void PmemIterator::SeekToFirst() {
    current_ = const_cast<PMEMoid *>(pmem_skiplist_->GetFirstOID(index_));
    assert(!OID_IS_NULL(*current_));
  }
  void PmemIterator::SeekToLast() {
    current_ = const_cast<PMEMoid *>(pmem_skiplist_->GetLastOID(index_));
    assert(!OID_IS_NULL(*current_));
  }
  void PmemIterator::Next() {
    struct skiplist_map_node *current_node = 
                          (struct skiplist_map_node *)pmemobj_direct(*current_);
    current_ = &(current_node->next[0].oid); // just move to next oid
    if (OID_IS_NULL(*current_)) {
      printf("[ERROR][PmemIterator][Next] OID IS NULL\n");
    }
  }
  void PmemIterator::Prev() {
    struct skiplist_map_node *current_node = 
                          (struct skiplist_map_node *)pmemobj_direct(*current_);
    char *key =  (char *)pmemobj_direct(current_node->entry.key);
    current_ = const_cast<PMEMoid *>(pmem_skiplist_->GetPrevOID(index_, key));
    // FIXME: check Prev() to -1
    if (OID_IS_NULL(*current_)) {
      printf("[ERROR][PmemIterator][Prev] OID IS NULL\n");
    }
  }


  bool PmemIterator::Valid() const {
    // printf("[ERROR][PmemIterator][Valid] OID IS NULL\n");
    if (OID_IS_NULL(*current_)) return false;
    struct skiplist_map_node *current_node = 
                          (struct skiplist_map_node *)pmemobj_direct(*current_);
    uint8_t key_len = current_node->entry.key_len;
    uint8_t value_len = current_node->entry.value_len;
    return key_len && value_len;
  }
  // TODO: Minimize seek time
  Slice PmemIterator::key() const {
    assert(!OID_IS_NULL(*current_));
    struct skiplist_map_node *current_node = 
                          (struct skiplist_map_node *)pmemobj_direct(*current_);
    uint8_t key_len = current_node->entry.key_len;
    void *ptr = pmemobj_direct(current_node->entry.key);
    Slice res((char *)ptr, key_len);
    return res;
  }
  Slice PmemIterator::value() const {
    assert(!OID_IS_NULL(*current_));
    struct skiplist_map_node *current_node = 
                          (struct skiplist_map_node *)pmemobj_direct(*current_);
    uint8_t value_len = current_node->entry.value_len;
    void *ptr = pmemobj_direct(current_node->entry.value);
    Slice res((char *)ptr, value_len);
    return res;
  }
  Status PmemIterator::status() const {
    Status s;
    // if (Valid()) {
    //   s = Status::OK();
    // } else {
    //   s = Status::NotFound(Slice());
    //   printf("[ERROR][PmemIterator][Status()]\n");
    // }
    return s;
  }


} // namespace leveldb 