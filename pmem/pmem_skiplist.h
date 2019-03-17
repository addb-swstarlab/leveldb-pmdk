/*
 * [2019.03.10][JH]
 * PBAC(Persistent Byte-Adressable Compaction) skiplist
 */
#include <list>
#include <map>

#include "pmem/layout.h"
#include "pmem/ds/skiplist_key_ptr.h"
#include "pmem/map/hashmap.h"

#include "leveldb/iterator.h"
#include "util/coding.h" 

// C++
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/pool.hpp>

// TEST:
#include "pmem/ds/hashmap_atomic.h"

// use pmem with c++ bindings
namespace pobj = pmem::obj;

namespace leveldb {

  struct skiplist_map_entry;
  struct skiplist_map_node;     // Skiplist Actual node 
  struct root_skiplist;         // Skiplist head
  struct root_skiplist_manager; //  Manager of Multiple Skiplists

  // PBuf
  struct root_pmem_buffer;
  class PmemBuffer;

  // TEST: hashmap
  struct root_hashmap;
  struct root_hashmap_manager;

  /* 
   * Dynamic allocation
   * Common function about list, map
   */
  // Free-list
  void PushFreeList(std::list<uint64_t>* free_list, uint64_t index);  // push free-index
  uint64_t PopFreeList(std::list<uint64_t>* free_list);             // return first free-index
  // Allocated-map
  void InsertAllocatedMap(std::map<uint64_t, uint64_t>* allocated_map, 
                          uint64_t file_number, uint64_t index);
  uint64_t GetIndexFromAllocatedMap(std::map<uint64_t, uint64_t>* allocated_map,
                                    uint64_t file_number); // return index
  void EraseAllocatedMap(std::map<uint64_t, uint64_t>* allocated_map, 
                          uint64_t file_number);
  bool CheckMapValidation(std::map<uint64_t, uint64_t>* allocated_map, 
                          uint64_t file_number);
  // Control functions
  uint64_t AddFileAndGetNewIndex(std::list<uint64_t>* free_list,
                                 std::map<uint64_t, uint64_t>* allocated_map,
                                 uint64_t file_number);
  void DeleteFile(std::list<uint64_t>* free_list,
                  std::map<uint64_t, uint64_t>* allocated_map,
                  uint64_t file_number);

  // TEST:
  void EncodeToBuffer(std::string* buffer, const Slice& key, const Slice& value);
  void AddToPmemBuffer(PmemBuffer* pmem_buffer, std::string* buffer, uint64_t file_number);
  uint32_t PrintKVAndReturnLength(char* buf);
  void GetAndPrintAll(PmemBuffer* pmem_buffer, uint64_t file_number);
  uint32_t SkipNEntriesAndGetOffset(const char* buf, uint64_t file_number, uint8_t n);
  std::string GetKeyFromBuffer(char* buf);
  char* GetValueFromBuffer(char* buf, uint32_t* value_length);
  int GetEncodedLength(const size_t key_size, const size_t value_size);

  class PmemSkiplist {
   public:
    PmemSkiplist();
    PmemSkiplist(std::string pool_path);
    ~PmemSkiplist();
    void Init(std::string pool_path);
    
    /* Getter */
    PMEMobjpool* GetPool();

    /* Wrapper functions */
    void Insert(char* key, char* buffer_ptr, 
                      int key_len, uint64_t file_number);
    void InsertByOID(PMEMoid* key_oid, char* buffer_ptr, int key_len, 
                      uint64_t file_number);
    void InsertByPtr(void* key_ptr, char* buffer_ptr, int key_len, 
                      uint64_t file_number);
    void InsertNullNode(uint64_t file_number);
    // char* Get(int index, char *key);
    void Foreach(uint64_t file_number, int (*callback) (
                char* key, char* buffer_ptr, int key_len, void* arg));
    void PrintAll(uint64_t file_number);
    void ClearAll();

    /* Iterator functions */
    PMEMoid* GetPrevOID(uint64_t file_number, char* key);
    PMEMoid* GetNextOID(uint64_t file_number, char* key);
    PMEMoid* GetFirstOID(uint64_t file_number);    
    PMEMoid* GetLastOID(uint64_t file_number);

   private:
    struct root_skiplist* root_skiplist_map_;

    /* Actual Skiplist interface */
    TOID(struct skiplist_map_node)* skiplists_;
    // TOID(struct skiplist_map_node) *current_node[SKIPLIST_MANAGER_LIST_SIZE];
    TOID(struct skiplist_map_node)* current_node;
    
    pobj::pool<root_skiplist_manager> skiplist_pool;
    pobj::persistent_ptr<root_skiplist_manager> root_skiplist_;

    /* Dynamic allocation */
    std::list<uint64_t> free_list_;
    std::map<uint64_t, uint64_t> allocated_map_; // [ file_number -> index ]
  };
  
  /* SOLVE: Pmem-based Iterator */
  class PmemIterator: public Iterator {
   public:
    PmemIterator(PmemSkiplist* pmem_skiplist); // For internal-iterator
    PmemIterator(int index, PmemSkiplist* pmem_skiplist); // For temp compaction
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
    // SOLVE:
    virtual void* key_ptr() const;
    // FIXME: modify all value_ptr to buffer_ptr
    virtual char* buffer_ptr() const;
    
    PMEMoid* GetCurrentOID();
    struct skiplist_map_node* GetCurrentNode();
    void SetIndex(int index);
    int GetIndex();

   private:
    PmemSkiplist* pmem_skiplist_;
    int index_;
    PMEMoid* current_;
    struct skiplist_map_node* current_node_;

    mutable PMEMoid* key_oid_;
    mutable PMEMoid* value_oid_;
    // SOLVE:
    mutable void* key_ptr_;
    mutable char* buffer_ptr_;
  };


  class PmemBuffer {
   public:
    PmemBuffer();
    PmemBuffer(std::string pool_path);
    ~PmemBuffer();
    void Init(std::string pool_path);
    void ClearAll();

    /* Read/Write function */
    void SequentialWrite(uint64_t file_number, const Slice& data);
    void RandomRead(uint64_t file_number, 
                    uint64_t offset, size_t n, Slice* result);
    std::string key(char* buf) const;
    std::string value(char* buf) const;

    /* Getter */
    PMEMobjpool* GetPool();
    char* SetAndGetStartOffset(uint64_t file_number);

   private:
    pobj::pool<root_pmem_buffer> buffer_pool_;
    pobj::persistent_ptr<root_pmem_buffer> root_buffer_;
    /* Dynamic allocation */
    std::list<uint64_t> free_list_;
    std::map<uint64_t, uint64_t> allocated_map_; // [ file_number -> index ]
  };
  struct root_pmem_buffer {
    pobj::persistent_ptr<char[]> contents;
    pobj::persistent_ptr<uint32_t[]> contents_size;
  };



  // TEST:
  class PmemHashmap {
   public:
    PmemHashmap();
    PmemHashmap(std::string pool_path);
    ~PmemHashmap();
    void Init(std::string pool_path);
    
    /* Getter */
    PMEMobjpool* GetPool();

    /* Wrapper functions */
    void Insert(uint64_t file_number, uint64_t key, PMEMoid value);
    void Foreach(uint64_t file_number,
        int (*callback)(uint64_t key, PMEMoid value, void *arg));
    void PrintAll(uint64_t file_number);
    // void Insert(char* key, char* buffer_ptr, 
    //                   int key_len, uint64_t file_number);
    // void InsertByPtr(void* key_ptr, char* buffer_ptr, int key_len, 
    //                   uint64_t file_number);
    // void InsertNullNode(uint64_t file_number);
    // // char* Get(int index, char *key);
    // void Foreach(uint64_t file_number, int (*callback) (
    //             char* key, char* buffer_ptr, int key_len, void* arg));
    // void PrintAll(uint64_t file_number);
    // void ClearAll();

    // /* Iterator functions */
    // PMEMoid* GetPrevOID(uint64_t file_number, char* key);
    // PMEMoid* GetNextOID(uint64_t file_number, char* key);
    // PMEMoid* GetFirstOID(uint64_t file_number);    
    // PMEMoid* GetLastOID(uint64_t file_number);

   private:
    struct root_hashmap* root_hashmap_;

    /* Actual Skiplist interface */
    TOID(struct hashmap_atomic)* hashmap_;
    // TOID(struct skiplist_map_node) *current_node[SKIPLIST_MANAGER_LIST_SIZE];
    // TOID(struct hashmap_atomic)* current_node;
    
    pobj::pool<root_hashmap_manager> hashmap_pool;
    pobj::persistent_ptr<root_hashmap_manager> root_hashmap_ptr_;

  };

} // namespace leveldb