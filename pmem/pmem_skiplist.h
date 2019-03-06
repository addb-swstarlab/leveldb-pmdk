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
    char* Get(int index, char *key);

    /* Iterator functions */
    const PMEMoid* GetPrevOID(int index, char *key);
    const PMEMoid* GetNextOID(int index, char *key);
    const PMEMoid* GetFirstOID(int index);    
    const PMEMoid* GetLastOID(int index);
    // TOID(struct skiplist_map_node)* GetTOID(int index, char *key);

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

   private:
    PmemSkiplist *pmem_skiplist_;
    int index_;
    // Status status_;
    PMEMoid* current_;
  };

} // namespace leveldb