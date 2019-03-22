/*
 * [2019.03.21][JH]
 * PMDK-based hashmap class
 * Need to optimize some functions...
 */

#include <iostream>
#include <fstream>
#include "pmem/pmem_hashmap.h"

namespace leveldb {
  /* Structure for hashmap */
  struct entry {
    PMEMoid key;
    uint8_t key_len;
    void* key_ptr;
    char* buffer_ptr;

    /* list pointer */
    POBJ_LIST_ENTRY(struct entry) list;
    POBJ_LIST_ENTRY(struct entry) iterator;
  };
  struct entry_args {
    PMEMoid key;
    uint8_t key_len;
    void* key_ptr;
    char* buffer_ptr;
  };
  POBJ_LIST_HEAD(entries_head, struct entry);
  struct buckets {
    /* number of buckets */
    size_t nbuckets;
    /* array of lists */
    struct entries_head bucket[];
  };
  POBJ_LIST_HEAD(iterator_head, struct entry);
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
    struct iterator_head entries;
  };
  struct root_hashmap { // head node
    TOID(struct hashmap_atomic) head;
  };
  struct root_hashmap_manager {
    pobj::persistent_ptr<root_hashmap[]> hashmap;
  };

  /* PMDK-based hashmap class */
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
              pobj::make_persistent<root_hashmap[]>(HASHMAP_LIST_SIZE);
      });
      root_hashmap_ = (struct root_hashmap *)pmemobj_direct(
         root_hashmap_ptr_->hashmap.raw() );
      hashmap_ = (TOID(struct hashmap_atomic) *) malloc(
            sizeof(TOID(struct hashmap_atomic)) * HASHMAP_LIST_SIZE);

      // printf("Create\n");
      for (int i=0; i < HASHMAP_LIST_SIZE; i++) {
        hm_atomic_create(GetPool(), &root_hashmap_[i].head, nullptr);
        hashmap_[i] = root_hashmap_[i].head;
      }
    } 
    // Not exists
    else {
      hashmap_pool = pobj::pool<root_hashmap_manager>::open (
                      pool_path, pool_path);
      root_hashmap_ptr_ = hashmap_pool.get_root();
      root_hashmap_ = (struct root_hashmap *)pmemobj_direct(
         root_hashmap_ptr_->hashmap.raw() );

      hashmap_ = (TOID(struct hashmap_atomic) *) malloc(
            sizeof(TOID(struct hashmap_atomic)) * HASHMAP_LIST_SIZE);
      
      // FIXME: Check test
      for (int i=0; i < HASHMAP_LIST_SIZE; i++) {
        // hm_atomic_create(GetPool(), &root_hashmap_[i].head, nullptr);
        hm_atomic_init(GetPool(), root_hashmap_[i].head);
        hashmap_[i] = root_hashmap_[i].head;
      }
    }
  }
  void PmemHashmap::Insert(char* key, char* buffer_ptr, 
                           int key_len, uint64_t file_number) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, 
                                                  file_number);
    int result = hm_atomic_insert(GetPool(), 
                                  hashmap_[actual_index], 
                                  key, buffer_ptr,
                                  key_len);
    if(result) { 
      fprintf(stderr, "[ERROR] insert %d\n", file_number);  
      abort();
    } 
  }
  void PmemHashmap::InsertByPtr(void* key_ptr, char* buffer_ptr, 
                                      int key_len, uint64_t file_number) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, 
                                                  file_number);
    int result = hm_atomic_insert_by_ptr(GetPool(), 
                                          hashmap_[actual_index], 
                                          key_ptr, buffer_ptr,
                                          key_len);
    if(result) { 
      fprintf(stderr, "[ERROR] insert_by_oid %d\n", file_number);  
      abort();
    } 
  }
  void PmemHashmap::Foreach(uint64_t file_number,
        int (*callback)(char* key, char* buffer_ptr, void* key_ptr, 
                        int key_len, void *arg)) {
    int res = hm_atomic_foreach(GetPool(), 
                                  hashmap_[file_number], callback, nullptr);
  }
  static int
  Print_hashmap(char* key, char* buffer_ptr, void* key_ptr, int key_len, void *arg)
  {
    if (key_ptr == nullptr) {
      printf("[print1] [key %d]:'%s', [value]:'%s'\n", key_len, key, buffer_ptr);
    } else {
      printf("[print2] [key %d]:'%s', [value]:'%s'\n", key_len, key_ptr, buffer_ptr);
    }
    return 0;
  }
  void PmemHashmap::PrintAll(uint64_t file_number) {
    Foreach(file_number, Print_hashmap);
  }
  void PmemHashmap::ClearAll() {
    for (int i=0; i<HASHMAP_LIST_SIZE; i++) {
      // skiplist_map_clear(GetPool(), skiplists_[i]);
      // DA: Push all to freelist
      PushFreeList(&free_list_, i);
    }
  }
  

  PMEMoid* PmemHashmap::GetPrevOID(uint64_t file_number, TOID(struct entry) current_entry) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, 
                                                  file_number);
    // return hm_atomic_get_prev_OID(GetPool(), hashmap_[actual_index], key);
    return hm_atomic_get_prev_OID(GetPool(), current_entry);
  }
  PMEMoid* PmemHashmap::GetNextOID(uint64_t file_number, TOID(struct entry) current_entry) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, 
                                                  file_number);
    return hm_atomic_get_next_OID(GetPool(), current_entry);
  }
  PMEMoid* PmemHashmap::GetFirstOID(uint64_t file_number) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, 
                                                  file_number);
    return hm_atomic_get_first_OID(GetPool(), hashmap_[actual_index]);
  }
  PMEMoid* PmemHashmap::GetLastOID(uint64_t file_number) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, 
                                                  file_number);
    return hm_atomic_get_last_OID(GetPool(), hashmap_[actual_index]);
  }
  PMEMoid* PmemHashmap::SeekOID(uint64_t file_number, char* key, int key_len) {
    uint64_t actual_index = GetActualIndex(&free_list_, &allocated_map_, 
                                                  file_number);
    return hm_atomic_seek_OID(GetPool(), hashmap_[actual_index], key, key_len);
  }

} // namespace leveldb 