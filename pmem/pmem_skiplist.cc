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
    // Offsets
  };
  PmemSkiplist::PmemSkiplist() {
    Init(SKIPLIST_MANAGER_PATH);
  }
  PmemSkiplist::PmemSkiplist(std::string pool_path) {
    Init(pool_path);
  }
  PmemSkiplist::~PmemSkiplist() {
    printf("destructor\n");
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
      /* 
      * 3) map_create for all indices
      */
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
      // pool = pmemobj_open(pool_path.c_str(), 
                                      // POBJ_LAYOUT_NAME(root_skiplist_manager));
      skiplist_pool = pobj::pool<root_skiplist_manager>::open(pool_path, pool_path);

      // if (pool == NULL) {
      //   printf("[ERROR] pmemobj_create\n");
      //   abort();
		  // }
      // root = POBJ_ROOT(pool, struct root_skiplist_manager);
      root_skiplist_ = skiplist_pool.get_root();
      root_skiplist_map_ = (struct root_skiplist *)pmemobj_direct(
         root_skiplist_->skiplists.raw() );
      // root_skiplist_map_ = (struct root_skiplist *)pmemobj_direct(
      //    class_root_skiplist_->skiplist_ptr.raw() );


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
                            int value_len, int index) {
    int result = skiplist_map_insert(GetPool(), 
                                      skiplists_[index], 
                                      &current_node[index],
                                      key, value,
                                      key_len, value_len, index);
    if(result) { 
      fprintf(stderr, "[ERROR] insert %d\n", index);  
      abort();
    } 
  }
  void PmemSkiplist::InsertByOID(PMEMoid *key_oid, PMEMoid *value_oid, 
                                      int key_len, int value_len, int index) {
    int result = skiplist_map_insert_by_oid(GetPool(), 
                                      skiplists_[index], 
                                      &current_node[index],
                                      key_oid, value_oid,
                                      key_len, value_len, index);
    if(result) { 
      fprintf(stderr, "[ERROR] insert_by_oid %d\n", index);  
      abort();
    } 
  }
  void PmemSkiplist::InsertNullNode(int index) {
    int result = skiplist_map_insert_null_node(GetPool(),
                                skiplists_[index], &current_node[index], index);
    if(result) {
      fprintf(stderr, "[ERROR] insert_null_node %d\n", index);  
    }
  }
  // NOTE: Will be deprecated.. 
  // char* PmemSkiplist::Get(int index, char *key) {
  //   return skiplist_map_get(GetPool(), skiplists_[index], key);
  // }
  void PmemSkiplist::Foreach(int index,
        int (*callback)(char *key, char *value, int key_len, int value_len, void *arg)) {
    int res = skiplist_map_foreach(GetPool(), 
                                  skiplists_[index], callback, nullptr);
  }
  void PmemSkiplist::PrintAll(int index) {
    Foreach(index, Print_skiplist);
  }
  void PmemSkiplist::ClearAll() {
    for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
      skiplist_map_clear(GetPool(), skiplists_[i]);
    }
  }

  // SOLVE: Implement Internal GetTOID
  const PMEMoid* PmemSkiplist::GetPrevOID(int index, char *key) {
    // int actual_index = index / 10;
    return skiplist_map_get_prev_OID(GetPool(), skiplists_[index], key);
  }
  PMEMoid* PmemSkiplist::GetNextOID(int index, char *key) {
    // int actual_index = index / 10;
    return skiplist_map_get_next_OID(GetPool(), skiplists_[index], key);
  }
  const PMEMoid* PmemSkiplist::GetFirstOID(int index) {
    // int actual_index = index / 10;
    return skiplist_map_get_first_OID(GetPool(), skiplists_[index]);
  }
  const PMEMoid* PmemSkiplist::GetLastOID(int index) {
    // int actual_index = index / 10;
    return skiplist_map_get_last_OID(GetPool(), skiplists_[index]);
  }


  /* SOLVE: Pmem-based Iterator */
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
    current_ = const_cast<PMEMoid *>(pmem_skiplist_->GetNextOID(index_, (char *)target.data()));
    current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
  }
  void PmemIterator::SeekToFirst() {
    current_ = const_cast<PMEMoid *>(pmem_skiplist_->GetFirstOID(index_));
    current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
    assert(!OID_IS_NULL(*current_));
  }
  void PmemIterator::SeekToLast() {
    current_ = const_cast<PMEMoid *>(pmem_skiplist_->GetLastOID(index_));
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
    current_ = const_cast<PMEMoid *>(pmem_skiplist_->GetPrevOID(index_, key));
    current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
    // FIXME: check Prev() to -1
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
    key_oid_ = &(current_node_->entry.key); // mutable
    void *ptr = pmemobj_direct(*key_oid_);
    Slice res((char *)ptr, key_len);
    return res;
  }
  Slice PmemIterator::value() const {
    assert(!OID_IS_NULL(*current_));
    uint8_t value_len = current_node_->entry.value_len;
    value_oid_ = &(current_node_->entry.value); // mutable
    void *ptr = pmemobj_direct(*value_oid_);
    Slice res((char *)ptr, value_len);
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

  PMEMoid* PmemIterator::GetCurrentOID() {
    return current_;
  }


} // namespace leveldb 