/*
 * [2019.03.10][JH]
 * PMDK-based iterator class
 * For pmem_skiplist and pmem_hashmap
 */
#include <iostream>
#include "pmem/pmem_iterator.h"

namespace leveldb {
  /* Structure for skiplist */
  struct skiplist_map_entry {
    char* buffer_ptr;
  };
  struct skiplist_map_node {
    TOID(struct skiplist_map_node) next[SKIPLIST_LEVELS_NUM];
    struct skiplist_map_entry entry;
  };
  /* Structure for Hashmap */
  struct entry {
    PMEMoid key;
    uint8_t key_len;
    void* key_ptr;
    char* buffer_ptr;
    /* list pointer */
    POBJ_LIST_ENTRY(struct entry) list;
    POBJ_LIST_ENTRY(struct entry) iterator;
  };

  /* 
   * Pmem-based Iterator 
   */
  PmemIterator::PmemIterator(PmemSkiplist *pmem_skiplist) 
    : index_(0), pmem_skiplist_(pmem_skiplist), data_structure(kSkiplist) {
  }
  PmemIterator::PmemIterator(int index, PmemSkiplist *pmem_skiplist) 
    : index_(index), pmem_skiplist_(pmem_skiplist), data_structure(kSkiplist) {
      // printf("[Constructor]New Iterator From Pmem %d\n", index_);
  }
  PmemIterator::PmemIterator(PmemHashmap* pmem_hashmap) 
    : index_(0), pmem_hashmap_(pmem_hashmap), data_structure(kHashmap) {
  }
  PmemIterator::PmemIterator(int index, PmemHashmap* pmem_hashmap) 
    : index_(index), pmem_hashmap_(pmem_hashmap), data_structure(kHashmap) {
  }
  PmemIterator::~PmemIterator() {
    // printf("PmemIterator destructor %d\n", index_);
      // printf("[Destructor]Delete Iterator %d\n", index_);
  }

  void PmemIterator::Seek(const Slice& target) {
    if (data_structure == kSkiplist) {
      current_ = (pmem_skiplist_->GetOID(index_, (char *)target.data()));
      SetCurrentNode(current_);
    } else if (data_structure == kHashmap) {
      current_ = pmem_hashmap_->SeekOID(index_, (char *)target.data(), 
                                target.size());
      SetCurrentEntry(current_);
    }
  }
  void PmemIterator::SeekToFirst() {
    if (data_structure == kSkiplist) {
      current_ = (pmem_skiplist_->GetFirstOID(index_));
      assert(!OID_IS_NULL(*current_));
      SetCurrentNode(current_);
    } else if (data_structure == kHashmap) {
      current_ = pmem_hashmap_->GetFirstOID(index_);
      assert(!OID_IS_NULL(*current_));
      SetCurrentEntry(current_);
    }
  }
  void PmemIterator::SeekToLast() {
    if (data_structure == kSkiplist) {
      current_ = (pmem_skiplist_->GetLastOID(index_));
      assert(!OID_IS_NULL(*current_));
      SetCurrentNode(current_);
    } else if (data_structure == kHashmap) {
      current_ = pmem_hashmap_->GetLastOID(index_);
      assert(!OID_IS_NULL(*current_));
      SetCurrentEntry(current_);
    }
  }
  void PmemIterator::Next() {
    if (data_structure == kSkiplist) {
      current_ = &(current_node_->next[0].oid); // just move to next oid
      if (OID_IS_NULL(*current_)) {
        printf("[ERROR][PmemIterator][Next] OID IS NULL\n");
      }
      SetCurrentNode(current_);
    } else if (data_structure == kHashmap) {
      TOID(struct entry) current_toid(*current_);
      current_ = pmem_hashmap_->GetNextOID(index_, current_toid);
      if (OID_IS_NULL(*current_)) {
        printf("[ERROR][PmemIterator][Next] OID IS NULL\n");
      }
      SetCurrentEntry(current_);
    }
  }
  void PmemIterator::Prev() {
    if (data_structure == kSkiplist) {
      char* key =  (char *)key_ptr_;
      current_ = (pmem_skiplist_->GetPrevOID(index_, key));
      if (OID_IS_NULL(*current_)) {
        printf("[ERROR][PmemIterator][Prev] OID IS NULL\n");
      }
      SetCurrentNode(current_);
    } else if (data_structure == kHashmap) {
      TOID(struct entry) current_toid(*current_);
      current_ = pmem_hashmap_->GetPrevOID(index_, current_toid);
      if (OID_IS_NULL(*current_)) {
        printf("[ERROR][PmemIterator][Next] OID IS NULL\n");
      }
      SetCurrentEntry(current_);
    }
  }

  bool PmemIterator::Valid() const {
    if (data_structure == kSkiplist) {
      if (OID_IS_NULL(*current_)) {
        printf("[Valid()]OID IS NULL\n");
        return false;
      }
      if (current_node_->entry.buffer_ptr == nullptr) {
        return false;
      }
      uint8_t key_len = GetKeyLengthFromBuffer(current_node_->entry.buffer_ptr);
      return key_len;
    } else if (data_structure == kHashmap) {
      if (OID_IS_NULL(*current_)) return false;
      uint8_t key_len = current_entry_->key_len;
      return key_len;
    }
  }
  Slice PmemIterator::key() const {
    if (data_structure == kSkiplist) {
      assert(!OID_IS_NULL(*current_));
      uint32_t key_len;
      char* ptr = GetKeyAndLengthFromBuffer(current_node_->entry.buffer_ptr, &key_len);
      key_ptr_ = ptr;
      buffer_ptr_ = current_node_->entry.buffer_ptr;
      Slice res((char *)ptr, key_len);
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
        key_ptr_ = pmemobj_direct_latency(*key_oid_);
        buffer_ptr_ = current_entry_->buffer_ptr;
        ptr = key_ptr_;
      }
      Slice res((char *)ptr, key_len);
      return res;
    }
  }
  Slice PmemIterator::value() const {
    if (data_structure == kSkiplist) {
      assert(!OID_IS_NULL(*current_));
      uint32_t value_len;
      char* ptr = GetValueAndLengthFromBuffer(buffer_ptr_, &value_len);
      Slice res((char *)ptr, value_len);
      return res;
    } else if (data_structure == kHashmap) {
      // TODO: Implement hashmap-based value()
    }
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
  int PmemIterator::GetIndex() {
    return index_;
  }
  void PmemIterator::SetIndex(int index) {
    index_ = index;
  }
  void PmemIterator::SetCurrentNode(PMEMoid* current_oid) {
    current_node_ = (struct skiplist_map_node*)pmemobj_direct_latency(*current_oid);
  }
  void PmemIterator::SetCurrentEntry(PMEMoid* current_oid) {
    current_entry_ = (struct entry*)pmemobj_direct_latency(*current_oid);
  }
} // namespace leveldb 