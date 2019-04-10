/*
 * [2019.03.10][JH]
 * PMDK-based iterator class
 * For pmem_skiplist and pmem_hashmap
 */
#ifndef PMEM_ITERATOR_H
#define PMEM_ITERATOR_H

// #include "pmem/layout.h"
#include "leveldb/iterator.h"
// C++
// #include <libpmemobj++/persistent_ptr.hpp>
// #include <libpmemobj++/make_persistent_array.hpp>
// #include <libpmemobj++/make_persistent.hpp>
// #include <libpmemobj++/transaction.hpp>
// #include <libpmemobj++/pool.hpp>

#include "pmem/pmem_skiplist.h"
#include "pmem/pmem_hashmap.h"
#include "pmem/ds/skiplist_buffer.h" // Use Getter from buffer
// #include "pmem/pmem_buffer.h"

// use pmem with c++ bindings
namespace pobj = pmem::obj;

namespace leveldb {

  /* Structure */
  struct skiplist_map_entry;
  struct skiplist_map_node; 
  struct entry;

  // Choose data-structure options
  enum PmemDataStructrueType {
    kSkiplist = 0,
    kHashmap = 1
  };

  /* 
   * Pmem-based Iterator 
   * Support skiplist-based and hashmap-based iterator
   * TODO: hashmap-based value()
   */
  class PmemIterator: public Iterator {
   public:
    PmemIterator(PmemSkiplist* pmem_skiplist); 
    PmemIterator(int index, PmemSkiplist* pmem_skiplist); 
    PmemIterator(PmemHashmap* pmem_hashmap);
    PmemIterator(int index, PmemHashmap* pmem_hashmap); 
    ~PmemIterator();

    void Seek(const Slice& target);
    void SeekToFirst();
    void SeekToLast();
    void Next();
    void Prev();

    bool Valid() const;
    Slice key() const;
    Slice value() const;
    Status status() const;

    virtual PMEMoid* key_oid() const;
    virtual PMEMoid* value_oid() const;

    virtual void* key_ptr() const;
    virtual char* buffer_ptr() const;
    
    /* Setter and Getter */
    PMEMoid* GetCurrentOID();
    struct skiplist_map_node* GetCurrentNode();
    int GetIndex();
    void SetIndex(int index);
    void SetCurrentNode(PMEMoid* current_oid);  // for skiplist
    void SetCurrentEntry(PMEMoid* current_oid); // for hashmap

   private:
    PmemSkiplist* pmem_skiplist_;
    PmemHashmap* pmem_hashmap_;

    int index_; // storing actual index
    PMEMoid* current_;
    struct skiplist_map_node* current_node_; // for skiplist
    struct entry* current_entry_;            // for hashmap

    mutable PMEMoid* key_oid_;
    mutable PMEMoid* value_oid_;

    mutable void* key_ptr_;
    mutable char* buffer_ptr_;

    const PmemDataStructrueType data_structure; 

  };
 
} // namespace leveldb

#endif