/*
 * 
 */

#include <iostream>
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
  Print_skiplist(char* key, char* value, int key_len, void *arg)
  {
    // if (strcmp(key, "") != 0 && strcmp(value, "") != 0)
    //   printf("[print] [key %d]:'%s', [value %d]:'%s'\n", strlen(key), key, strlen(value), value);
    // TODO: Get value from value_ptr
    if (strcmp(key, "") != 0)
      printf("[print] [key %d]:'%s', [value]:'%s'\n", key_len, key, value);
    delete[] key;
    // delete[] value;
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
  void DeleteFile(std::list<uint64_t>* free_list,
                  std::map<uint64_t, uint64_t>* allocated_map,
                  uint64_t file_number) {
    uint64_t old_index = GetIndexFromAllocatedMap(allocated_map, file_number);
    EraseAllocatedMap(allocated_map, file_number);
    PushFreeList(free_list, old_index);
  }
  
  // NOTE: For Iterator
  struct skiplist_map_entry {
    PMEMoid key;
    uint8_t key_len;
    void* key_ptr;
    // TEST:
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
      root_skiplist_ = skiplist_pool.get_root();
      pobj::transaction::exec_tx(skiplist_pool, [&] {
        // Make 100 skiplists
        root_skiplist_->skiplists = 
              pobj::make_persistent<root_skiplist[]>(SKIPLIST_MANAGER_LIST_SIZE);
      });
      root_skiplist_map_ = (struct root_skiplist *)pmemobj_direct(
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

  void PmemSkiplist::Insert(char* key, char* buffer_ptr, int key_len, 
                            uint64_t file_number) {
    uint64_t actual_index = CheckMapValidation(&allocated_map_, file_number) ? 
                GetIndexFromAllocatedMap(&allocated_map_, file_number) :
                AddFileAndGetNewIndex(&free_list_, &allocated_map_, file_number);
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
  void PmemSkiplist::InsertByOID(PMEMoid* key_oid, char* buffer_ptr, 
                                      int key_len, uint64_t file_number) {
    uint64_t actual_index = CheckMapValidation(&allocated_map_, file_number) ? 
                GetIndexFromAllocatedMap(&allocated_map_, file_number) :
                AddFileAndGetNewIndex(&free_list_, &allocated_map_, file_number);
    int result = skiplist_map_insert_by_oid(GetPool(), 
                                      skiplists_[actual_index], 
                                      &current_node[actual_index],
                                      key_oid, buffer_ptr,
                                      key_len, actual_index);
    if(result) { 
      fprintf(stderr, "[ERROR] insert_by_oid %d\n", file_number);  
      abort();
    } 
  }
  void PmemSkiplist::InsertByPtr(void* key_ptr, char* buffer_ptr, 
                                      int key_len, uint64_t file_number) {
    uint64_t actual_index = CheckMapValidation(&allocated_map_, file_number) ? 
                GetIndexFromAllocatedMap(&allocated_map_, file_number) :
                AddFileAndGetNewIndex(&free_list_, &allocated_map_, file_number);
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
    uint64_t actual_index = CheckMapValidation(&allocated_map_, file_number) ? 
                GetIndexFromAllocatedMap(&allocated_map_, file_number) :
                AddFileAndGetNewIndex(&free_list_, &allocated_map_, file_number);
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
    uint64_t actual_index = CheckMapValidation(&allocated_map_, file_number) ? 
                GetIndexFromAllocatedMap(&allocated_map_, file_number) :
                AddFileAndGetNewIndex(&free_list_, &allocated_map_, file_number);
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
      PushFreeList(&free_list_, i);
    }
  }

  // SOLVE: Implement Internal GetTOID
  PMEMoid* PmemSkiplist::GetPrevOID(uint64_t file_number, char* key) {
    uint64_t actual_index = CheckMapValidation(&allocated_map_, file_number) ? 
                GetIndexFromAllocatedMap(&allocated_map_, file_number) :
                AddFileAndGetNewIndex(&free_list_, &allocated_map_, file_number);
    return skiplist_map_get_prev_OID(GetPool(), skiplists_[actual_index], key);
  }
  PMEMoid* PmemSkiplist::GetNextOID(uint64_t file_number, char* key) {
    uint64_t actual_index = CheckMapValidation(&allocated_map_, file_number) ? 
                GetIndexFromAllocatedMap(&allocated_map_, file_number) :
                AddFileAndGetNewIndex(&free_list_, &allocated_map_, file_number);
    return skiplist_map_get_next_OID(GetPool(), skiplists_[actual_index], key);
  }
  PMEMoid* PmemSkiplist::GetFirstOID(uint64_t file_number) {
    uint64_t actual_index = CheckMapValidation(&allocated_map_, file_number) ? 
                GetIndexFromAllocatedMap(&allocated_map_, file_number) :
                AddFileAndGetNewIndex(&free_list_, &allocated_map_, file_number);
    return skiplist_map_get_first_OID(GetPool(), skiplists_[actual_index]);
  }
  PMEMoid* PmemSkiplist::GetLastOID(uint64_t file_number) {
    uint64_t actual_index = CheckMapValidation(&allocated_map_, file_number) ? 
                GetIndexFromAllocatedMap(&allocated_map_, file_number) :
                AddFileAndGetNewIndex(&free_list_, &allocated_map_, file_number);
    return skiplist_map_get_last_OID(GetPool(), skiplists_[actual_index]);
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
    // printf("PmemIterator destructor %d\n", index_);
  }

  // TODO: Implement index block to reduce unnecessary seek time
  void PmemIterator::SetIndexAndSeek(int index, const Slice& target) {
    SetIndex(index);
    // printf("SetIndexAndSeek %d\n", index_);
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
    char* key =  (char *)pmemobj_direct(current_node_->entry.key);
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
    return key_len;
  }
  // TODO: Minimize seek time
  Slice PmemIterator::key() const {
    // printf("00]\n");
    assert(!OID_IS_NULL(*current_));
    uint8_t key_len = current_node_->entry.key_len;
    void *ptr;
    // printf("11]\n");
    if (current_node_->entry.key_ptr != nullptr &&
        current_node_->entry.buffer_ptr != nullptr) {
      key_ptr_ = current_node_->entry.key_ptr;
      buffer_ptr_ = current_node_->entry.buffer_ptr; // Already set for value
      ptr = key_ptr_;
    } else {
      key_oid_ = &(current_node_->entry.key); // mutable
      key_ptr_ = pmemobj_direct(*key_oid_);
      buffer_ptr_ = current_node_->entry.buffer_ptr; // Already set for value
      ptr = key_ptr_;
    }
    // printf("33]\n");
    Slice res((char *)ptr, key_len);
    // printf("key:'%s'\n", res.data());
    return res;
  }
  // PROGRESS: implement PmemBuffer-based Get-Value
  Slice PmemIterator::value() const {
    assert(!OID_IS_NULL(*current_));
    uint32_t value_length;
    char* ptr = GetValueFromBuffer(buffer_ptr_, &value_length);
    // Slice res(value.c_str(), value.size());
    Slice res((char *)ptr, value_length);
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
  char* PmemIterator::buffer_ptr() const {
    return buffer_ptr_;
  }

  PMEMoid* PmemIterator::GetCurrentOID() {
    return current_;
  }
  struct skiplist_map_node* PmemIterator::GetCurrentNode() {
    return current_node_;
  }
  void PmemIterator::SetIndex(int index) {
    index_ = index;
  }
  // DEBUG:
  int PmemIterator::GetIndex() {
    return index_;
  }

  /* Pmem-based buffer */
  PmemBuffer::PmemBuffer() {
    Init(BUFFER_PATH);
  }
  PmemBuffer::PmemBuffer(std::string pool_path) {
    Init(pool_path);
  }
  PmemBuffer::~PmemBuffer() {
    buffer_pool_.close();
  }
  void PmemBuffer::Init(std::string pool_path) {
    if (!file_exists(pool_path)) {
      buffer_pool_ = pobj::pool<root_pmem_buffer>::create (
                      pool_path, pool_path, 
                      (unsigned long)BUFFER_POOL_SIZE, 0666);
      root_buffer_ = buffer_pool_.get_root();

      pobj::transaction::exec_tx(buffer_pool_, [&] {
        root_buffer_->contents = 
              pobj::make_persistent<char[]>(MAX_CONTENTS_SIZE);
        root_buffer_->contents_size =
              pobj::make_persistent<uint32_t[]>(NUM_OF_CONTENTS);
      });
    } 
    // Not exists
    else {
      buffer_pool_ = pobj::pool<root_pmem_buffer>::open (
                      pool_path, pool_path);
      root_buffer_ = buffer_pool_.get_root();
    }
  }
  void PmemBuffer::ClearAll() {
    // Fill free_list
    
    for (int index=0; index<NUM_OF_CONTENTS; index++) {
      // Clear all contents_size
      uint32_t initial_zero = 0;
      buffer_pool_.memcpy_persist(
            root_buffer_->contents_size.get() + (index * sizeof(uint32_t)), 
            &initial_zero,
            sizeof(uint32_t));

      // DA: Push all to freelist
      PushFreeList(&free_list_, index);
    }
  }
  void PmemBuffer::SequentialWrite(uint64_t file_number, const Slice& data) {
    // Get offset(index)
    uint64_t index = (uint32_t) GetIndexFromAllocatedMap(&allocated_map_, file_number);

    uint32_t data_size = data.size();
    // Sequential-Write(memcpy) from buf to specific contents offset
    buffer_pool_.memcpy_persist(
      root_buffer_->contents.get() + (index * sizeof(char) * EACH_CONTENT_SIZE), 
      data.data(), 
      data_size
    );

    // Set contents_size about matching offset(index)
    // buffer_pool_.memcpy_persist(
    //   root_buffer_->contents_size.get() + (index * sizeof(uint32_t)),
    //   &data_size,
    //   sizeof(uint32_t)
    // );
  }
  void PmemBuffer::RandomRead(uint64_t file_number,
                              uint64_t offset, size_t n, Slice* result) {
    // Get offset(index)
    // + Invaild check (Before read sst, it has been finished write)
    if(!CheckMapValidation(&allocated_map_, file_number)) {
      printf("[ERROR] %d is not in allocated_map...\n",file_number);
      abort();
    }
    uint32_t index = (uint32_t) GetIndexFromAllocatedMap(&allocated_map_, file_number);

    // Check whether offset+size is over contents_size
    uint32_t contents_size;
    memcpy(&contents_size, root_buffer_->contents_size.get() + (index * sizeof(uint32_t)), 
            sizeof(uint32_t));
    if (offset + n > contents_size) {
      // Overflow
      printf("[WARN][PmemBuffer][RandomRead] Read overflow\n");
      // abort();
    }
    
    // Make result Slice
    *result = Slice( root_buffer_->contents.get() + 
                     (index * sizeof(char) * EACH_CONTENT_SIZE) + offset, 
                     n);
  }
  /* 
   * TEST: actual function will be in GetFromPmem()
   * NOTE: Be copied from memtable.cc
   */
  std::string PmemBuffer::key(char* buf) const {
    // Read encoded key-length
    uint32_t key_length;
    const char* key_ptr = GetVarint32Ptr(buf, buf+5, &key_length);
    // Get key
    return std::string(key_ptr, key_length);
  }
  std::string PmemBuffer::value(char* buf) const {
    // Read encoded key-length
    uint32_t key_length, value_length;
    // Skip key-part
    const char* key_ptr = GetVarint32Ptr(buf, buf+5, &key_length);
    // Read encoded value-length
    const char* value_ptr = GetVarint32Ptr(
                                    buf+key_length+VarintLength(key_length),
                                    buf+key_length+VarintLength(key_length)+5,
                                    &value_length);
    // Get value
    return std::string(value_ptr, value_length);                    
  }
  /* Getter */
  PMEMobjpool* PmemBuffer::GetPool() {
    return buffer_pool_.get_handle();
  }
  char* PmemBuffer::SetAndGetStartOffset(uint64_t file_number) {
    if(CheckMapValidation(&allocated_map_, file_number)) {
      printf("[WARNING] %d is already inserted into allocated-map\n", file_number);
      // abort();
    }
    uint64_t new_index = AddFileAndGetNewIndex(&free_list_, &allocated_map_, 
                                                file_number);

    return root_buffer_->contents.get() + (new_index * sizeof(char) * EACH_CONTENT_SIZE);
  }

  void EncodeToBuffer(std::string* buffer, const Slice& key, const Slice& value) {
    // printf("[Encode Debug] '%s' - '%s'\n", key.data(), value.data());
    PutLengthPrefixedSlice(buffer, key);
    // printf("[Encode1 %d] '%s'\n", buffer->size(), buffer->c_str());
    PutLengthPrefixedSlice(buffer, value);
    // printf("[Encode2 %d] '%s'\n", buffer->size(), buffer->c_str());
  }
  void AddToPmemBuffer(PmemBuffer* pmem_buffer, 
                                  std::string* buffer, uint64_t file_number) {
    pmem_buffer->SequentialWrite(file_number, Slice(*buffer));
  }

  // void Get(char* buf, const Slice& key, std::string* value) {
  uint32_t PrintKVAndReturnLength(char* buf) {
    Slice key_buffer(buf);
    Slice key, value;
    uint32_t decoded_len;
    GetLengthPrefixedSlice(&key_buffer, &key);
    decoded_len += VarintLength(key.size());
    decoded_len += key.size();
    Slice value_buffer(buf+decoded_len);
    GetLengthPrefixedSlice(&value_buffer, &value);
    decoded_len += VarintLength(value.size());
    decoded_len += value.size();

    std::string res_key(key.data(), key.size());
    std::string res_value(value.data(), value.size());

    printf("key:'%s'\n", res_key.c_str());
    printf("value:'%s'\n", res_value.c_str());


    printf("decoded_length %d\n", decoded_len);
    return decoded_len;
  }
  void GetAndPrintAll(PmemBuffer* pmem_buffer, uint64_t file_number) {
    Slice result;
    pmem_buffer->RandomRead(file_number, 0, EACH_CONTENT_SIZE, &result);

    uint32_t offset = 0;
    for (int i=0; i<10; i++) {
      printf("[%d]\n", i);
      offset += PrintKVAndReturnLength(const_cast<char *>(result.data()) + offset);
    }
  }
  uint32_t SkipNEntriesAndGetOffset(const char* buf, uint64_t file_number, uint8_t n) {
    Slice key, value;
    uint32_t skip_length = 0;
    uint32_t key_length, value_length;
    for (int i=0; i< n; i++) {
      char* tmp_buf = const_cast<char *>(buf)+skip_length;
      const char* key_ptr = GetVarint32Ptr(tmp_buf, tmp_buf+5, &key_length);
      uint32_t encoded_key_length = VarintLength(key_length) + key_length;
      const char* value_ptr = GetVarint32Ptr(tmp_buf+encoded_key_length, 
                                    tmp_buf+encoded_key_length+5, &value_length);
      uint32_t encoded_value_length = VarintLength(value_length) + value_length;
      skip_length += (encoded_key_length + encoded_value_length);
    }
    return skip_length;
  }
  std::string GetKeyFromBuffer(char* buf) {
    // Read encoded key-length
    uint32_t key_length;
    const char* key_ptr = GetVarint32Ptr(buf, buf+5, &key_length);
    // Get key
    return std::string(key_ptr, key_length);
  }
  char* GetValueFromBuffer(char* buf, uint32_t* value_len) {
    // Read encoded key-length
    uint32_t key_length, value_length;
    // Skip key-part
    const char* key_ptr = GetVarint32Ptr(buf, buf+5, &key_length);
    // Read encoded value-length
    const char* value_ptr = GetVarint32Ptr(
                                    buf+key_length+VarintLength(key_length),
                                    buf+key_length+VarintLength(key_length)+5,
                                    value_len);
    // Get value
    return const_cast<char *>(value_ptr);                 
  }
  int GetEncodedLength(const size_t key_size, const size_t value_size) {
    return VarintLength(key_size) + key_size + VarintLength(value_size) + value_size;
  }


  // TEST:
  TOID_DECLARE(struct buckets, HASHMAP_ATOMIC_TYPE_OFFSET + 1);
  TOID_DECLARE(struct entry, HASHMAP_ATOMIC_TYPE_OFFSET + 2);

  struct entry {
    uint64_t key;
    PMEMoid value;

    /* list pointer */
    POBJ_LIST_ENTRY(struct entry) list;
  };

  struct entry_args {
    uint64_t key;
    PMEMoid value;
  };

  POBJ_LIST_HEAD(entries_head, struct entry);
  struct buckets {
    /* number of buckets */
    size_t nbuckets;
    /* array of lists */
    struct entries_head bucket[];
  };

  struct hashmap_atomic {
    /* random number generator seed */
    uint32_t seed;

    /* hash function coefficients */
    uint32_t hash_fun_a;
    uint32_t hash_fun_b;
    uint64_t hash_fun_p;

    /* number of values inserted */
    uint64_t count;
    /* whether "count" should be updated */
    uint32_t count_dirty;

    /* buckets */
    TOID(struct buckets) buckets;
    /* buckets, used during rehashing, null otherwise */
    TOID(struct buckets) buckets_tmp;
  };


  struct root_hashmap { // head node
    TOID(struct hashmap_atomic) head;
  };
  struct root_hashmap_manager {
    pobj::persistent_ptr<root_hashmap> hashmap;
  };

  PmemHashmap::PmemHashmap() {
    Init(BUFFER_PATH);
  }
  PmemHashmap::PmemHashmap(std::string pool_path) {
    Init(pool_path);
  }
  PmemHashmap::~PmemHashmap() {
    hashmap_pool.close();
  }
  PMEMobjpool* PmemHashmap::GetPool() {
    return hashmap_pool.get_handle();
  }
  void PmemHashmap::Init(std::string pool_path) {
    if (!file_exists(pool_path)) {
      hashmap_pool = pobj::pool<root_hashmap_manager>::create (
                      pool_path, pool_path, 
                      (unsigned long)BUFFER_POOL_SIZE, 0666);
      root_hashmap_ptr_ = hashmap_pool.get_root();

      pobj::transaction::exec_tx(hashmap_pool, [&] {
        root_hashmap_ptr_->hashmap =
              pobj::make_persistent<root_hashmap>();
      });
      root_hashmap_ = (struct root_hashmap *)pmemobj_direct(
         root_hashmap_ptr_->hashmap.raw() );
      hashmap_ = (TOID(struct hashmap_atomic) *) malloc(
            sizeof(TOID(struct hashmap_atomic)) * 1);

      printf("Create\n");
      hm_atomic_create(GetPool(), &root_hashmap_[0].head, nullptr);
      hashmap_[0] = root_hashmap_[0].head;

    } 
    // Not exists
    else {
      hashmap_pool = pobj::pool<root_hashmap_manager>::open (
                      pool_path, pool_path);
      root_hashmap_ptr_ = hashmap_pool.get_root();
    }
  }
  // void PmemHashmap::Insert(uint64_t file_number, uint64_t key, PMEMoid value) {
  //   int res = hm_atomic_insert(GetPool(), hashmap_[file_number], key, value);
  //   if (res != 0) printf("ERROR Hashmap insertion\n");
  // }
  // void PmemHashmap::Foreach(uint64_t file_number,
  //       int (*callback)(uint64_t key, PMEMoid value, void *arg)) {
  //   int res = hm_atomic_foreach(GetPool(), 
  //                                 hashmap_[file_number], callback, nullptr);
  // }
  // static int
  // Print_hashmap(uint64_t key, PMEMoid value, void *arg)
  // {
  //   // if (strcmp(key, "") != 0 && strcmp(value, "") != 0)
  //   //   printf("[print] [key %d]:'%s', [value %d]:'%s'\n", strlen(key), key, strlen(value), value);
  //   // TODO: Get value from value_ptr
  //   // if (strcmp(key, "") != 0)
  //   char* value_ptr = (char *)pmemobj_direct(value);

  //   printf("[print] [key]:'%d', [value]:'%s'\n", key, value_ptr);
  //   // delete[] key;
  //   // delete[] value;
  //   return 0;
  // }
  // void PmemHashmap::PrintAll(uint64_t file_number) {
  //   Foreach(file_number, Print_hashmap);
  // }

} // namespace leveldb 