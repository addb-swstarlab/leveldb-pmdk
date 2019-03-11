/*
 * [2019.03.10][JH]
 * PBAC(Persistent Byte-Adressable Compaction) skiplist
 */
#include "pmem/layout.h"
#include "pmem/ds/skiplist_map.h"
#include "pmem/map/hashmap.h"

#include "leveldb/iterator.h"

// C++
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/pool.hpp>

// use pmem with c++ bindings
namespace pobj = pmem::obj;

namespace leveldb {

  struct skiplist_map_entry;
  struct skiplist_map_node;     // Skiplist Actual node 
  struct root_skiplist;         // Skiplist head
  struct root_skiplist_manager; //  Manager of Multiple Skiplists

  // PBuf
  struct root_pmem_buffer;

  class PmemSkiplist {
   public:
    PmemSkiplist();
    PmemSkiplist(std::string pool_path);
    ~PmemSkiplist();
    void Init(std::string pool_path);
    
    /* Getter */
    PMEMobjpool* GetPool();

    /* Wrapper functions */
    void Insert(char *key, char *value, int key_len, int value_len, int index);
    void InsertByOID(PMEMoid *key_oid, PMEMoid *value_oid, int key_len, 
                      int value_len, int index);
    void InsertNullNode(int index);
    // char* Get(int index, char *key);
    void Foreach(int index, 
        int (*callback)(char *key, char *value, int key_len, int value_len, void *arg));
    void PrintAll(int index);
    void ClearAll();

    /* Iterator functions */
    const PMEMoid* GetPrevOID(int index, char *key);
    PMEMoid* GetNextOID(int index, char *key);
    const PMEMoid* GetFirstOID(int index);    
    const PMEMoid* GetLastOID(int index);
 
   private:
    struct root_skiplist *root_skiplist_map_;

    /* Actual Skiplist interface */
    TOID(struct skiplist_map_node) *skiplists_;
    // TOID(struct skiplist_map_node) *current_node[SKIPLIST_MANAGER_LIST_SIZE];
    TOID(struct skiplist_map_node) *current_node;
    
    pobj::pool<root_skiplist_manager> skiplist_pool;
    pobj::persistent_ptr<root_skiplist_manager> root_skiplist_;
  };
  
  /* SOLVE: Pmem-based Iterator */
  class PmemIterator: public Iterator {
   public:
    PmemIterator(PmemSkiplist *pmem_skiplist); // For internal-iterator
    PmemIterator(int index, PmemSkiplist *pmem_skiplist); // For temp compaction
    ~PmemIterator();

    void SetIndexAndSeek(int index, const Slice& target);
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
    
    PMEMoid* GetCurrentOID();

   private:
    PmemSkiplist* pmem_skiplist_;
    int index_;
    PMEMoid* current_;
    struct skiplist_map_node* current_node_;

    mutable PMEMoid* key_oid_;
    mutable PMEMoid* value_oid_;
  };

  class PmemBuffer {
   public:
    PmemBuffer();
    ~PmemBuffer();
    void SequentialWrite(char* buf);
    void RandomRead(uint64_t offset, size_t n, Slice* result);

   private:
    pobj::pool<root_pmem_buffer> buffer_pool;
    pobj::persistent_ptr<root_pmem_buffer> root_buffer_;
  };
  struct root_pmem_buffer {
    pobj::persistent_ptr<char[]> contents;
    pobj::persistent_ptr<uint32_t[]> contents_size;
  };

} // namespace leveldb