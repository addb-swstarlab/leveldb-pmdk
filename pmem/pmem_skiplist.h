#include <list>
#include <map>
#include "pmem/layout.h"
// #include "pmem/map/map.h"
#include "pmem/map/map_skiplist.h"
#include "pmem/map/hashmap.h"

// C++
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/pool.hpp>

#include "leveldb/iterator.h"

// use pmem with c++ bindings
namespace pobj = pmem::obj;

namespace leveldb {
    POBJ_LAYOUT_BEGIN(root_skiplist_manager);
    POBJ_LAYOUT_ROOT(root_skiplist_manager, struct root_skiplist_manager);
    POBJ_LAYOUT_TOID(root_skiplist_manager, struct root_skiplist_map);
    // POBJ_LAYOUT_TOID(root_skiplist_manager, struct skiplist_map_node);
    POBJ_LAYOUT_END(root_skiplist_manager);
  struct skiplist_map_node;
  struct root_skiplist_manager;
  struct root_skiplist_map;
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
  uint64_t GetActualIndex(std::list<uint64_t>* free_list,
            std::map<uint64_t, uint64_t>* allocated_map, uint64_t file_number);

  class PmemSkiplist {
   public:
    PmemSkiplist();
    PmemSkiplist(std::string pool_path);
    ~PmemSkiplist();
    void Init(std::string pool_path);
    void ClearAll();
    
    /* Getter */
    PMEMobjpool* GetPool();

    /* Wrapper functions */
    void Insert(char *key, char *value, int key_len, int value_len, int index);
    void InsertByOID(PMEMoid* node, PMEMoid *key_oid, PMEMoid *value_oid, 
                      int key_len, int value_len, int index);
    char* Get(int index, char *key);

    /* Iterator functions */
    const PMEMoid* GetPrevOID(int index, char *key);
    const PMEMoid* GetNextOID(int index, char *key);
    const PMEMoid* GetFirstOID(int index);    
    const PMEMoid* GetLastOID(int index);

    void GetNextTOID(int index, char *key, 
                      TOID(struct map) *prev, TOID(struct map) *curr);

    size_t GetFreeListSize();
    size_t GetAllocatedMapSize();
    void DeleteFile(uint64_t file_number);

   private:
    PMEMobjpool *pool;
    pobj::pool<root_skiplist_manager> skiplist_pool;

    const struct map_ops *ops;

    TOID(struct root_skiplist_manager) root;
    struct root_skiplist_map *skiplists;
    
    /* Actual Skiplist interface */
    struct map_ctx *mapc;
    TOID(struct map) *map;
    TOID(struct map)* current_node;

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
    virtual PMEMoid* node() const;

    PMEMoid* GetCurrentOID();

   private:
    PmemSkiplist* pmem_skiplist_;
    int index_;
    // Status status_;
    PMEMoid* current_;
    struct skiplist_map_node* current_node_;

    mutable PMEMoid* key_oid_;
    mutable PMEMoid* value_oid_;
    // PROGRESS:
    // TOID(struct map) *prev_;
    // TOID(struct map) *curr_;
  };

} // namespace leveldb