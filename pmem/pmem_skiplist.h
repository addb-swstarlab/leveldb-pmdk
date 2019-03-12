/*
 * [2019.03.10][JH]
 * PBAC(Persistent Byte-Adressable Compaction) skiplist
 */
#include <list>
#include <map>

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
    void Insert(char *key, char *value, 
                      int key_len, int value_len, uint64_t file_number);
    void InsertByOID(PMEMoid *key_oid, PMEMoid *value_oid, int key_len, 
                      int value_len, uint64_t file_number);
    // TEST: temp insertion based on only ptr
    void InsertByPtr(void* key_ptr, void* value_ptr, int key_len, 
    // void InsertByPtr(PMEMoid *key_oid, PMEMoid *value_oid, int key_len, 
                      int value_len, uint64_t file_number);
    void InsertNullNode(uint64_t file_number);
    // char* Get(int index, char *key);
    void Foreach(uint64_t file_number, int (*callback) (
                char *key, char *value, int key_len, int value_len, void *arg));
    void PrintAll(uint64_t file_number);
    void ClearAll();

    /* Iterator functions */
    PMEMoid* GetPrevOID(uint64_t file_number, char *key);
    PMEMoid* GetNextOID(uint64_t file_number, char *key);
    PMEMoid* GetFirstOID(uint64_t file_number);    
    PMEMoid* GetLastOID(uint64_t file_number);
 
    /* Dynamic allocation */
    // Free-list
    void PushFreeList(uint64_t index);  // push free-index
    uint64_t PopFreeList();             // return first free-index
    // Allocated-map
    void InsertAllocatedMap(uint64_t file_number, uint64_t index);
    uint64_t GetIndexFromAllocatedMap(uint64_t file_number); // return index
    void EraseAllocatedMap(uint64_t file_number);
    bool CheckMapValidation(uint64_t file_number);
    // Control functions
    uint64_t AddFileAndGetNewIndex(uint64_t file_number);
    void DeleteFile(uint64_t file_number);

   private:
    struct root_skiplist *root_skiplist_map_;

    /* Actual Skiplist interface */
    TOID(struct skiplist_map_node) *skiplists_;
    // TOID(struct skiplist_map_node) *current_node[SKIPLIST_MANAGER_LIST_SIZE];
    TOID(struct skiplist_map_node) *current_node;
    
    pobj::pool<root_skiplist_manager> skiplist_pool;
    pobj::persistent_ptr<root_skiplist_manager> root_skiplist_;

    /* Dynamic allocation */
    std::list<uint64_t> free_list_;
    std::map<uint64_t, uint64_t> allocated_map_; // [ file_number -> index ]
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
    // TEST:
    virtual void* key_ptr() const;
    virtual void* value_ptr() const;
    
    PMEMoid* GetCurrentOID();

   private:
    PmemSkiplist* pmem_skiplist_;
    int index_;
    PMEMoid* current_;
    struct skiplist_map_node* current_node_;

    mutable PMEMoid* key_oid_;
    mutable PMEMoid* value_oid_;
    // TEST:
    mutable void* key_ptr_;
    mutable void* value_ptr_;
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