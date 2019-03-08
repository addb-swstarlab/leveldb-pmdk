#include "pmem/layout.h"
// #include "pmem/map/map.h"
#include "pmem/map/map_skiplist.h"
#include "pmem/map/hashmap.h"

#include "leveldb/iterator.h"

namespace leveldb {
    POBJ_LAYOUT_BEGIN(root_skiplist_manager);
    POBJ_LAYOUT_ROOT(root_skiplist_manager, struct root_skiplist_manager);
    POBJ_LAYOUT_TOID(root_skiplist_manager, struct root_skiplist_map);
    // POBJ_LAYOUT_TOID(root_skiplist_manager, struct skiplist_map_node);
    POBJ_LAYOUT_END(root_skiplist_manager);
  struct skiplist_map_node;
  struct root_skiplist_manager;
  struct root_skiplist_map;

  class PmemSkiplist {
   public:
    PmemSkiplist();
    ~PmemSkiplist();
    void Init();
    
    /* Getter */
    PMEMobjpool* GetPool();

    /* Wrapper functions */
    void Insert(char *key, char *value, int key_len, int value_len, int index);
    void InsertByOID(PMEMoid *key_oid, PMEMoid *value_oid, int key_len, 
                      int value_len, int index);
    char* Get(int index, char *key);

    /* Iterator functions */
    const PMEMoid* GetPrevOID(int index, char *key);
    const PMEMoid* GetNextOID(int index, char *key);
    const PMEMoid* GetFirstOID(int index);    
    const PMEMoid* GetLastOID(int index);

    void GetNextTOID(int index, char *key, 
                      TOID(struct map) *prev, TOID(struct map) *curr);

   private:
    PMEMobjpool *pool;
    const struct map_ops *ops;

    TOID(struct root_skiplist_manager) root;
    struct root_skiplist_map *skiplists;
    
    /* Actual Skiplist interface */
    struct map_ctx *mapc;
    TOID(struct map) *map;
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