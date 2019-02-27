#include "pmem/layout.h"
#include "pmem/map/map.h"
#include "pmem/map/map_skiplist.h"
#include "pmem/map/hashmap.h"

namespace leveldb {
    POBJ_LAYOUT_BEGIN(root_skiplist_manager);
    POBJ_LAYOUT_ROOT(root_skiplist_manager, struct root_skiplist_manager);
    POBJ_LAYOUT_TOID(root_skiplist_manager, struct root_skiplist_map);
    POBJ_LAYOUT_END(root_skiplist_manager);
  struct root_skiplist_manager;
  struct root_skiplist_map;
  class PmemSkiplist {
   public:
    PmemSkiplist();
    ~PmemSkiplist();
    void Init();
    /* Getter */
    PMEMobjpool* GetPool();

    /* Wrapper function */
    void Insert(char *key, char *value, int key_len, int value_len, int index);
    char* Get(int index, char *key);


   private:
    PMEMobjpool *pool;
    const struct map_ops *ops;

    TOID(struct root_skiplist_manager) root;
    struct root_skiplist_map *skiplists;
    // static TOID(struct root_skiplist_map) *skiplists;
    
    /* Actual Skiplist interface */
    struct map_ctx *mapc;
    TOID(struct map) *map;
    
    // static PMEMobjpool *pool;
    // const struct map_ops *ops;

    // static TOID(struct root_skiplist_manager) root;
    // static struct root_skiplist_map *skiplists;
    // // static TOID(struct root_skiplist_map) *skiplists;
    
    // /* Actual Skiplist interface */
    // static struct map_ctx *mapc;
    // static TOID(struct map) *map;
  };

}