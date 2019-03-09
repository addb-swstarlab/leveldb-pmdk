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
  PmemSkiplist::PmemSkiplist() {
    Init();
  }
  PmemSkiplist::~PmemSkiplist() {
    printf("destructor\n");
    free(map);
    pmemobj_close(pool);
  }
  void PmemSkiplist::Init() {
    ops = MAP_SKIPLIST;
    if(!file_exists(SKIPLIST_MANAGER_PATH)) {
      /* Initialize pool & map_ctx */
      pool = pmemobj_create(SKIPLIST_MANAGER_PATH, 
                  POBJ_LAYOUT_NAME(root_skiplist_manager), 
                  (unsigned long)SKIPLIST_MANAGER_POOL_SIZE, 0666); 
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
      /* create */
      for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
        int res = map_create(mapc, &(skiplists[i].map), i, &args); 
        if (res) printf("[CREATE ERROR %d] %d\n",i ,res);
        else if (i==SKIPLIST_MANAGER_LIST_SIZE-1) printf("[CREATE SUCCESS %d]\n",i);	
        map[i] = skiplists[i].map;
      }
      /* NOTE: Reset current node */
      for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
        int res = map_create(mapc, &map[i], i, nullptr); 
        if (res) printf("[CREATE RESET ERROR %d] %d\n",i ,res);
        else if (i==SKIPLIST_MANAGER_LIST_SIZE-1) printf("[CREATE RESET SUCCESS %d]\n",i);	
      }
    } else {
      pool = pmemobj_open(SKIPLIST_MANAGER_PATH, 
                                      POBJ_LAYOUT_NAME(root_skiplist_manager));
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

      for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
				map[i] = skiplists[i].map;
      }
      /* NOTE: Clear and Reset current node */
      for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
        map_clear(mapc, map[i]);
        int res = map_create(mapc, &map[i], i, nullptr); 
        if (res) printf("[CREATE RESET ERROR %d] %d\n",i ,res);
        else if (i==SKIPLIST_MANAGER_LIST_SIZE-1) printf("[CREATE RESET SUCCESS %d]\n",i);	
      }
    }
  }
  
  PMEMobjpool* PmemSkiplist::GetPool() {
    return pool;
  }

  void PmemSkiplist::Insert(char *key, char *value, int key_len, 
                            int value_len, int index) {
    int result = map_insert(mapc, map[index], key, value, key_len, 
                            value_len, index);
    if(result) { 
      fprintf(stderr, "[ERROR] insert %d\n", index);  
      abort();
    } 
  }
  void PmemSkiplist::InsertByOID(PMEMoid *key_oid, PMEMoid *value_oid, 
                                      int key_len, int value_len, int index) {
    int result = map_insert_by_oid(mapc, map[index], key_oid, value_oid, 
                                    key_len, value_len, index);
    if(result) { 
      fprintf(stderr, "[ERROR] insert_by_oid %d\n", index);  
      abort();
    } 
  }
  // NOTE: Will be deprecated.. 
  char* PmemSkiplist::Get(int index, char *key) {
    return map_get(mapc, map[index], key);
  }

  // SOLVE: Implement Internal GetTOID
  const PMEMoid* PmemSkiplist::GetPrevOID(int index, char *key) {
    return map_get_prev_OID(mapc, map[index], key);
  }
  const PMEMoid* PmemSkiplist::GetNextOID(int index, char *key) {
    return map_get_next_OID(mapc, map[index], key);
  }
  const PMEMoid* PmemSkiplist::GetFirstOID(int index) {
    return map_get_first_OID(mapc, map[index]);
  }
  const PMEMoid* PmemSkiplist::GetLastOID(int index) {
    return map_get_last_OID(mapc, map[index]);
  }
  void PmemSkiplist::GetNextTOID(int index, char *key, 
                                TOID(struct map) *prev, TOID(struct map) *curr) {
    map_get_next_TOID(mapc, map[index], key, prev, curr);
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
    current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
    if (OID_IS_NULL(*current_)) {
      printf("[ERROR][SetIndexAndSeek] Access Invalid node .. '%s'\n", target.data());
      abort();
    }
  }
  void PmemIterator::Seek(const Slice& target) {
    current_ = const_cast<PMEMoid *>(pmem_skiplist_->GetNextOID(index_, (char *)target.data()));
    current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
    if (OID_IS_NULL(*current_)) {
      printf("[ERROR][Seek] Access Invalid node .. '%s'\n", target.data());
      abort();
    }
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
    // struct skiplist_map_node *current_node = 
    //                       (struct skiplist_map_node *)pmemobj_direct(*current_);
    current_ = &(current_node_->next[0].oid); // just move to next oid
    current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
    if (OID_IS_NULL(*current_)) {
      printf("[ERROR][PmemIterator][Next] OID IS NULL\n");
    }
  }
  void PmemIterator::Prev() {
    // struct skiplist_map_node *current_node = 
    //                       (struct skiplist_map_node *)pmemobj_direct(*current_);
    char *key =  (char *)pmemobj_direct(current_node_->entry.key);
    current_ = const_cast<PMEMoid *>(pmem_skiplist_->GetPrevOID(index_, key));
    current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
    // FIXME: check Prev() to -1
    if (OID_IS_NULL(*current_)) {
      printf("[ERROR][PmemIterator][Prev] OID IS NULL\n");
    }
  }


  bool PmemIterator::Valid() const {
    // printf("[ERROR][PmemIterator][Valid] OID IS NULL\n");
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
    // printf("1\n");
    // struct skiplist_map_node *current_node = 
    //                       (struct skiplist_map_node *)pmemobj_direct(*current_);
    uint8_t key_len = current_node_->entry.key_len;
    key_oid_ = &(current_node_->entry.key); // mutable
    void *ptr = pmemobj_direct(*key_oid_);
    Slice res((char *)ptr, key_len);
    // printf("2\n");
    return res;
  }
  Slice PmemIterator::value() const {
    assert(!OID_IS_NULL(*current_));
    // printf("11\n");
    // struct skiplist_map_node *current_node = 
    //                       (struct skiplist_map_node *)pmemobj_direct(*current_);
    uint8_t value_len = current_node_->entry.value_len;
    value_oid_ = &(current_node_->entry.value); // mutable
    void *ptr = pmemobj_direct(*value_oid_);
    Slice res((char *)ptr, value_len);
    // printf("22\n");
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