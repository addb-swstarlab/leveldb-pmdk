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
  static int
  Print_skiplist(char *key, char *value, int key_len, int value_len, void *arg)
  {
    // if (strcmp(key, "") != 0 && strcmp(value, "") != 0)
    //   printf("[print] [key %d]:'%s', [value %d]:'%s'\n", strlen(key), key, strlen(value), value);
    if (strcmp(key, "") != 0 && strcmp(value, "") != 0)
      printf("[print] [key %d]:'%s', [value %d]:'%s'\n", key_len, key, value_len, value);
    delete[] key;
    delete[] value;
    return 0;
  }
  
  // NOTE: For Iterator
  struct skiplist_map_entry {
    PMEMoid key;
    uint8_t key_len;
    PMEMoid value;
    uint8_t value_len;
    // TEST:
    void* key_ptr;
    void* value_ptr;
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
    // PMEMoid skiplists;
    pobj::persistent_ptr<root_skiplist[]> skiplists;
  };
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
      // class_skiplist_pool = pobj::pool<root_PBACSkiplist>::create (
      //                 pool_path, pool_path, 
      //                 (unsigned long)SKIPLIST_MANAGER_POOL_SIZE, 0666);
      
      /* 
      * 1) Get root and make root ptr.
      *    skiplists' PMEMoid is oid of overall skiplists
      */
      root_skiplist_ = skiplist_pool.get_root();
      // class_root_skiplist_ = class_skiplist_pool.get_root();
      /*
      * 2) Initialize root structure
      *    It includes malloc overall skiplists
      */
      pobj::transaction::exec_tx(skiplist_pool, [&] {
        // Make 100 skiplists
        root_skiplist_->skiplists = 
              pobj::make_persistent<root_skiplist[]>(SKIPLIST_MANAGER_LIST_SIZE);
        
        // for (int i=0; i < SKIPLIST_MANAGER_LIST_SIZE; i++) {
        //   root_skiplist_->skiplists[i].head = 
        //       pobj::make_persistent<skiplist_map_node>(SKIPLIST_BULK_INSERT_NUM);
        // }
      });
      // pobj::transaction::exec_tx(class_skiplist_pool, [&] {
      //   // Make 100 skiplists
      //   class_root_skiplist_->skiplist_ptr = 
      //         pobj::make_persistent<PBACSkiplist[]>(SKIPLIST_MANAGER_LIST_SIZE);
        
      //   // for (int i=0; i < SKIPLIST_MANAGER_LIST_SIZE; i++) {
      //   //   class_root_skiplist_->skiplist_ptr[i] = 
      //   //       pobj::make_persistent<PBACSkiplist>(pool_path);
      //   // }
      // });
      root_skiplist_map_ = (struct root_skiplist *)pmemobj_direct(
         root_skiplist_->skiplists.raw() );
      // root_skiplist_map_ = (struct root_skiplist *)pmemobj_direct(
      //    class_root_skiplist_->skiplist_ptr.raw() );


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
      root_skiplist_map_ = (struct root_skiplist *)pmemobj_direct(
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
  
  PMEMobjpool* PmemSkiplist::GetPool() {
    return skiplist_pool.get_handle();
  }

  void PmemSkiplist::Insert(char *key, char *value, int key_len, 
                            int value_len, uint64_t file_number) {
    uint64_t actual_index = CheckMapValidation(file_number) ? 
                            GetIndexFromAllocatedMap(file_number) :
                            AddFileAndGetNewIndex(file_number);
    int result = skiplist_map_insert(GetPool(), 
                                      skiplists_[actual_index], 
                                      &current_node[actual_index],
                                      key, value,
                                      key_len, value_len, actual_index);
    if(result) { 
      fprintf(stderr, "[ERROR] insert %d\n", file_number);  
      abort();
    } 
  }
  void PmemSkiplist::InsertByOID(PMEMoid *key_oid, PMEMoid *value_oid, 
                                      int key_len, int value_len, 
                                      uint64_t file_number) {
    uint64_t actual_index = CheckMapValidation(file_number) ? 
                            GetIndexFromAllocatedMap(file_number) :
                            AddFileAndGetNewIndex(file_number);
    int result = skiplist_map_insert_by_oid(GetPool(), 
                                      skiplists_[actual_index], 
                                      &current_node[actual_index],
                                      key_oid, value_oid,
                                      key_len, value_len, actual_index);
    if(result) { 
      fprintf(stderr, "[ERROR] insert_by_oid %d\n", file_number);  
      abort();
    } 
  }
  void PmemSkiplist::InsertByPtr(void* key_ptr, void* value_ptr, 
                                      int key_len, int value_len, 
                                      uint64_t file_number) {
    uint64_t actual_index = CheckMapValidation(file_number) ? 
                            GetIndexFromAllocatedMap(file_number) :
                            AddFileAndGetNewIndex(file_number);                                    
    int result = skiplist_map_insert_by_ptr(GetPool(), 
                                      skiplists_[actual_index], 
                                      &current_node[actual_index],
                                      key_ptr, value_ptr,
                                      key_len, value_len, actual_index);
    if(result) { 
      fprintf(stderr, "[ERROR] insert_by_oid %d\n", file_number);  
      abort();
    } 
  }
  void PmemSkiplist::InsertNullNode(uint64_t file_number) {
    uint64_t actual_index = CheckMapValidation(file_number) ? 
                            GetIndexFromAllocatedMap(file_number) :
                            AddFileAndGetNewIndex(file_number);   
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
        int (*callback)(char *key, char *value, int key_len, int value_len, void *arg)) {
    uint64_t actual_index = CheckMapValidation(file_number) ? 
                            GetIndexFromAllocatedMap(file_number) :
                            AddFileAndGetNewIndex(file_number);
    int res = skiplist_map_foreach(GetPool(), 
                                  skiplists_[actual_index], callback, nullptr);
  }
  void PmemSkiplist::PrintAll(uint64_t file_number) {
    Foreach(file_number, Print_skiplist);
  }
  void PmemSkiplist::ClearAll() {
    for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
      skiplist_map_clear(GetPool(), skiplists_[i]);
      // DA: Push all to freelist
      PushFreeList(i);
    }
  }

  // SOLVE: Implement Internal GetTOID
  PMEMoid* PmemSkiplist::GetPrevOID(uint64_t file_number, char *key) {
    uint64_t actual_index = CheckMapValidation(file_number) ? 
                            GetIndexFromAllocatedMap(file_number) :
                            AddFileAndGetNewIndex(file_number);
    return skiplist_map_get_prev_OID(GetPool(), skiplists_[actual_index], key);
  }
  PMEMoid* PmemSkiplist::GetNextOID(uint64_t file_number, char *key) {
    uint64_t actual_index = CheckMapValidation(file_number) ? 
                            GetIndexFromAllocatedMap(file_number) :
                            AddFileAndGetNewIndex(file_number);
    return skiplist_map_get_next_OID(GetPool(), skiplists_[actual_index], key);
  }
  PMEMoid* PmemSkiplist::GetFirstOID(uint64_t file_number) {
    uint64_t actual_index = CheckMapValidation(file_number) ? 
                            GetIndexFromAllocatedMap(file_number) :
                            AddFileAndGetNewIndex(file_number);
    return skiplist_map_get_first_OID(GetPool(), skiplists_[actual_index]);
  }
  PMEMoid* PmemSkiplist::GetLastOID(uint64_t file_number) {
    uint64_t actual_index = CheckMapValidation(file_number) ? 
                            GetIndexFromAllocatedMap(file_number) :
                            AddFileAndGetNewIndex(file_number);
    return skiplist_map_get_last_OID(GetPool(), skiplists_[actual_index]);
  }

  /* PROGRESS: Dynamic allocation */
  // Free-list
  void PmemSkiplist::PushFreeList(uint64_t index) {
    free_list_.push_back(index);
  } 
  uint64_t PmemSkiplist::PopFreeList() {
    // return free_list_
    if (free_list_.size() == 0) { 
      printf("[ERROR] free_list is empty... :( \n");
      abort();
    }
    uint16_t res = free_list_.front();
    free_list_.pop_front();
    return res;
  }
  // Allocated-map
  void PmemSkiplist::InsertAllocatedMap(uint64_t file_number, uint64_t index) {
    allocated_map_.emplace(file_number, index);
    if (allocated_map_.size() >= SKIPLIST_MANAGER_LIST_SIZE) {
      printf("[WARNING][InsertAllocatedMap] map size is full...\n");
    }
  }
  uint64_t PmemSkiplist::GetIndexFromAllocatedMap(uint64_t file_number) {
    std::map<uint64_t, uint64_t>::iterator iter = allocated_map_.find(file_number);
    if (iter == allocated_map_.end()) {
      printf("[WARNING][GetAllocatedMap] Cannot get %d from allocated map\n", file_number);
    }
    return iter->second;
  }
  void PmemSkiplist::EraseAllocatedMap(uint64_t file_number) {
    int res = allocated_map_.erase(file_number);
    if (!res) {
      printf("[WARNING][EraseAllocatedMap] fail to erase %d\n", file_number);
    }
  }
  bool PmemSkiplist::CheckMapValidation(uint64_t file_number) {
    std::map<uint64_t, uint64_t>::iterator iter = allocated_map_.find(file_number);
    if (iter == allocated_map_.end()) {
      return false;
    }
    return true;
  }
  // Control functions
  uint64_t PmemSkiplist::AddFileAndGetNewIndex(uint64_t file_number) {
    uint64_t new_index = PopFreeList();
    InsertAllocatedMap(file_number, new_index);
    return new_index;
  }
  void PmemSkiplist::DeleteFile(uint64_t file_number) {
    uint64_t old_index = GetIndexFromAllocatedMap(file_number);
    EraseAllocatedMap(file_number);
    PushFreeList(old_index);
  }


  /* 
   * Pmem-based Iterator 
   * NOTE: Do not touch iterator's index.
   */
  PmemIterator::PmemIterator(PmemSkiplist *pmem_skiplist) 
    : index_(0), pmem_skiplist_(pmem_skiplist) {
  }
  PmemIterator::PmemIterator(int index, PmemSkiplist *pmem_skiplist) 
    : index_(index), pmem_skiplist_(pmem_skiplist) {
  }
  PmemIterator::~PmemIterator() {
    
  }

  // TODO: Implement index block to reduce unnecessary seek time
  void PmemIterator::SetIndexAndSeek(int index, const Slice& target) {
    index_ = index;
    current_ = (pmem_skiplist_->GetNextOID(index_, (char *)target.data()));
    current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
  }
  void PmemIterator::Seek(const Slice& target) {
    current_ = (pmem_skiplist_->GetNextOID(index_, (char *)target.data()));
    current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
  }
  void PmemIterator::SeekToFirst() {
    current_ = (pmem_skiplist_->GetFirstOID(index_));
    current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
    assert(!OID_IS_NULL(*current_));
  }
  void PmemIterator::SeekToLast() {
    current_ = (pmem_skiplist_->GetLastOID(index_));
    current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
    assert(!OID_IS_NULL(*current_));
  }
  void PmemIterator::Next() {
    current_ = &(current_node_->next[0].oid); // just move to next oid
    current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
    if (OID_IS_NULL(*current_)) {
      printf("[ERROR][PmemIterator][Next] OID IS NULL\n");
    }
  }
  void PmemIterator::Prev() {
    char *key =  (char *)pmemobj_direct(current_node_->entry.key);
    current_ = (pmem_skiplist_->GetPrevOID(index_, key));
    current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
    if (OID_IS_NULL(*current_)) {
      printf("[ERROR][PmemIterator][Prev] OID IS NULL\n");
    }
  }


  bool PmemIterator::Valid() const {
    if (OID_IS_NULL(*current_)) return false;
    // struct skiplist_map_node *current_node = 
    //                       (struct skiplist_map_node *)pmemobj_direct(*current_);
    uint8_t key_len = current_node_->entry.key_len;
    uint8_t value_len = current_node_->entry.value_len;
    return key_len && value_len;
  }
  // TODO: Minimize seek time
  Slice PmemIterator::key() const {
    assert(!OID_IS_NULL(*current_));
    uint8_t key_len = current_node_->entry.key_len;
    void *ptr;
    if (current_node_->entry.key_ptr != nullptr &&
        current_node_->entry.value_ptr != nullptr) {
      key_ptr_ = current_node_->entry.key_ptr;
      ptr = key_ptr_;
    } else {
      key_oid_ = &(current_node_->entry.key); // mutable
      key_ptr_ = pmemobj_direct(*key_oid_);
      ptr = key_ptr_;
    }
    Slice res((char *)ptr, key_len);
    // printf("key:'%s'\n", res.data());
    return res;
  }
  Slice PmemIterator::value() const {
    assert(!OID_IS_NULL(*current_));
    uint8_t value_len = current_node_->entry.value_len;
    void *ptr;
    if (current_node_->entry.key_ptr != nullptr &&
        current_node_->entry.value_ptr != nullptr) {
      value_ptr_ = current_node_->entry.value_ptr;
      ptr = value_ptr_;
    } else {
      value_oid_ = &(current_node_->entry.value); // mutable
      value_ptr_ = pmemobj_direct(*value_oid_); 
      ptr = value_ptr_;
    }
    Slice res((char *)ptr, value_len);
    // printf("value:'%s'\n", res.data());
    return res;
  }
  Status PmemIterator::status() const {
    return Status::OK();
  }
  PMEMoid* PmemIterator::key_oid() const {
    return key_oid_;
  }
  PMEMoid* PmemIterator::value_oid() const {
    return value_oid_;
  }
  void* PmemIterator::key_ptr() const {
    return key_ptr_;
  }
  void* PmemIterator::value_ptr() const {
    return value_ptr_;
  }

  PMEMoid* PmemIterator::GetCurrentOID() {
    return current_;
  }


} // namespace leveldb 