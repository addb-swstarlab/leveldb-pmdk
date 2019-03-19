/*
 * 
 */

#include <iostream>
#include <fstream>
#include "pmem/pmem_iterator.h"
#define SKIPLIST_LEVELS_NUM 12 // redefine


namespace leveldb {
  inline bool
  file_exists (const std::string &name)
  {
    std::ifstream f (name.c_str ());
    return f.good ();
  }


  struct skiplist_map_entry {
    // PMEMoid key;
    // uint8_t key_len;
    // void* key_ptr;
    // TEST:
    char* buffer_ptr;
  };
  struct skiplist_map_node {
    TOID(struct skiplist_map_node) next[SKIPLIST_LEVELS_NUM];
    struct skiplist_map_entry entry;
  };
  struct entry {
    PMEMoid key;
    uint8_t key_len;
    void* key_ptr;
    char* buffer_ptr;

    /* list pointer */
    POBJ_LIST_ENTRY(struct entry) list;
    POBJ_LIST_ENTRY(struct entry) iterator;
  };
  // TOID_DECLARE(struct entry, HASHMAP_ATOMIC_TYPE_OFFSET + 2);
  /* 
   * Pmem-based Iterator 
   * NOTE: Do not touch iterator's index.
   */
  PmemIterator::PmemIterator(PmemSkiplist *pmem_skiplist) 
    : index_(0), pmem_skiplist_(pmem_skiplist), data_structure(kSkiplist) {
  }
  PmemIterator::PmemIterator(int index, PmemSkiplist *pmem_skiplist) 
    : index_(index), pmem_skiplist_(pmem_skiplist), data_structure(kSkiplist) {
  }
  PmemIterator::PmemIterator(PmemHashmap* pmem_hashmap) 
    : index_(0), pmem_hashmap_(pmem_hashmap), data_structure(kHashmap) {
  }
  PmemIterator::PmemIterator(int index, PmemHashmap* pmem_hashmap) 
    : index_(index), pmem_hashmap_(pmem_hashmap), data_structure(kHashmap) {
  }
  PmemIterator::~PmemIterator() {
    // printf("PmemIterator destructor %d\n", index_);
  }

  // TODO: Implement index block to reduce unnecessary seek time
  void PmemIterator::SetIndexAndSeek(int index, const Slice& target) {
    SetIndex(index);
    // printf("SetIndexAndSeek %d\n", index_);
    Seek(target);
    // if (data_structure == kSkiplist) {
    //   current_ = (pmem_skiplist_->GetNextOID(index_, (char *)target.data()));
    //   current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
    // } else if (data_structure == kHashmap) {
    //   current_ = pmem_hashmap_->SeekOID(index_, (char *)target.data(), 
    //                             target.size());
    //   current_entry_ = (struct entry *) pmemobj_direct(*current_);
    // }
  }
  void PmemIterator::Seek(const Slice& target) {
    // printf("Seek\n");
    if (data_structure == kSkiplist) {
      current_ = (pmem_skiplist_->GetNextOID(index_, (char *)target.data()));
      current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
    } else if (data_structure == kHashmap) {
      current_ = pmem_hashmap_->SeekOID(index_, (char *)target.data(), 
                                target.size());
      current_entry_ = (struct entry *) pmemobj_direct(*current_);
    }
    // printf("Seek End\n");
  }
  void PmemIterator::SeekToFirst() {
    // printf("SeekToFirst\n");
    if (data_structure == kSkiplist) {
      current_ = (pmem_skiplist_->GetFirstOID(index_));
      assert(!OID_IS_NULL(*current_));
      current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
    } else if (data_structure == kHashmap) {
      current_ = pmem_hashmap_->GetFirstOID(index_);
      assert(!OID_IS_NULL(*current_));
      current_entry_ = (struct entry *) pmemobj_direct(*current_);
    }
    // printf("SeekToFirst End\n");
    
  }
  void PmemIterator::SeekToLast() {
    if (data_structure == kSkiplist) {
      current_ = (pmem_skiplist_->GetLastOID(index_));
      assert(!OID_IS_NULL(*current_));
      current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
    } else if (data_structure == kHashmap) {
      current_ = pmem_hashmap_->GetLastOID(index_);
      assert(!OID_IS_NULL(*current_));
      current_entry_ = (struct entry *) pmemobj_direct(*current_);
    }
  }
  void PmemIterator::Next() {
    // printf("Next\n");
    if (data_structure == kSkiplist) {
      current_ = &(current_node_->next[0].oid); // just move to next oid
      if (OID_IS_NULL(*current_)) {
        printf("[ERROR][PmemIterator][Next] OID IS NULL\n");
      }
      current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
    } else if (data_structure == kHashmap) {
      TOID(struct entry) current_toid(*current_);
      current_ = pmem_hashmap_->GetNextOID(index_, current_toid);
      if (OID_IS_NULL(*current_)) {
        printf("[ERROR][PmemIterator][Next] OID IS NULL\n");
      }
      current_entry_ = (struct entry *)pmemobj_direct(*current_);
    }
    // printf("Next end\n");
  }
  void PmemIterator::Prev() {
    if (data_structure == kSkiplist) {
      // char* key =  (char *)pmemobj_direct(current_node_->entry.key);
      char* key =  (char *)key_ptr_;
      current_ = (pmem_skiplist_->GetPrevOID(index_, key));
      if (OID_IS_NULL(*current_)) {
        printf("[ERROR][PmemIterator][Prev] OID IS NULL\n");
      }
      current_node_ = (struct skiplist_map_node *)pmemobj_direct(*current_);
    } else if (data_structure == kHashmap) {
      TOID(struct entry) current_toid(*current_);
      current_ = pmem_hashmap_->GetPrevOID(index_, current_toid);
      if (OID_IS_NULL(*current_)) {
        printf("[ERROR][PmemIterator][Next] OID IS NULL\n");
      }
      current_entry_ = (struct entry *)pmemobj_direct(*current_);
    }
  }


  bool PmemIterator::Valid() const {
    // printf("Valid\n");
    if (data_structure == kSkiplist) {
      if (OID_IS_NULL(*current_)) {
        printf("[Valid()]OID IS NULL\n");
        return false;
      }
      if (current_node_->entry.buffer_ptr == nullptr) {
        // printf("Is not valid\n");
        return false;
      }
      // struct skiplist_map_node *current_node = 
      //                       (struct skiplist_map_node *)pmemobj_direct(*current_);
      uint8_t key_len = GetKeyLengthFromBuffer(current_node_->entry.buffer_ptr);
    // printf("Valid End\n");
      return key_len;
    } else if (data_structure == kHashmap) {
      if (OID_IS_NULL(*current_)) return false;
      uint8_t key_len = current_entry_->key_len;
      return key_len;
    }
  }
  // TODO: Minimize seek time
  Slice PmemIterator::key() const {
    // printf("00]\n");
    // printf("Key\n");
    if (data_structure == kSkiplist) {
      assert(!OID_IS_NULL(*current_));
      uint32_t key_len;
      // printf("11]\n");
      char* ptr = GetKeyAndLengthFromBuffer(current_node_->entry.buffer_ptr, &key_len);
      // if (current_node_->entry.key_ptr != nullptr &&
      //     current_node_->entry.buffer_ptr != nullptr) {
      //   key_ptr_ = current_node_->entry.key_ptr;
      //   buffer_ptr_ = current_node_->entry.buffer_ptr; // Already set for value
      //   ptr = key_ptr_;
      // } else {
      //   // printf("key_oid_\n");
      //   key_oid_ = &(current_node_->entry.key); // mutable
      //   key_ptr_ = pmemobj_direct(*key_oid_);
      //   buffer_ptr_ = current_node_->entry.buffer_ptr; // Already set for value
      //   ptr = key_ptr_;
      // }
      key_ptr_ = ptr;
      buffer_ptr_ = current_node_->entry.buffer_ptr;
      // printf("33]\n");
      Slice res((char *)ptr, key_len);
      // printf("key:'%s'\n", res.data());
    // printf("Key End\n");
      return res;
    } else if (data_structure == kHashmap) {
      assert(!OID_IS_NULL(*current_));
      uint8_t key_len = current_entry_->key_len;
      void* ptr;
      if (current_entry_->key_ptr != nullptr &&
          current_entry_->buffer_ptr != nullptr) {
        key_ptr_ = current_entry_->key_ptr;
        buffer_ptr_ = current_entry_->buffer_ptr;
        ptr = key_ptr_;
      } else {
        key_oid_ = &(current_entry_->key);
        key_ptr_ = pmemobj_direct(*key_oid_);
        buffer_ptr_ = current_entry_->buffer_ptr;
        ptr = key_ptr_;
      }
      Slice res((char *)ptr, key_len);
      return res;
    }
  }
  // PROGRESS: implement PmemBuffer-based Get-Value
  Slice PmemIterator::value() const {
    // if (data_structure == kSkiplist) {
    // printf("Value\n");
      assert(!OID_IS_NULL(*current_));
      uint32_t value_len;
      char* ptr = GetValueAndLengthFromBuffer(buffer_ptr_, &value_len);
      // Slice res(value.c_str(), value.size());
      Slice res((char *)ptr, value_len);
      // printf("value:'%s'\n", res.data());
    // printf("Value End\n");
      return res;
    // } else if (data_structure == kHashmap) {
    //   assert(!OID_IS_NULL(*current_));
    //   uint32_t value_len;
    // }
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
    if (data_structure == kSkiplist) {
      return current_node_;
    } else if (data_structure == kHashmap) {
      printf("[ERROR][GetCurrentNode] Hashmap is operated\n");
      abort();
    }
  }
  void PmemIterator::SetIndex(int index) {
    index_ = index;
  }
  // DEBUG:
  int PmemIterator::GetIndex() {
    return index_;
  }
} // namespace leveldb 