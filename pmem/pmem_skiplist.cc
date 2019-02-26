/*
 * 
 */

#include <fstream>
#include "pmem/pmem_skiplist.h"

namespace leveldb {
  inline bool
  file_exists (const std::string &name)
  {
    std::ifstream f (name.c_str ());
    return f.good ();
  }

  // Register root-manager & node structrue
  
  // Skiplist single-node
  struct root_skiplist_map {
    TOID(struct map) map;
  };
  // Skiplists manager
  struct root_skiplist_manager {
    int count;
    PMEMoid skiplists;
    // Offsets
  };
  PmemSkiplist::PmemSkiplist() {
    Init();
  };
  PmemSkiplist::~PmemSkiplist() {
    printf("destructor\n");
    free(map);
    pmemobj_close(pool);
  };
  void PmemSkiplist::Init() {
    ops = MAP_SKIPLIST;
    if(!file_exists(SKIPLIST_MANAGER_PATH)) {
      /* Initialize pool & map_ctx */
      pool = pmemobj_create(SKIPLIST_MANAGER_PATH, 
                  POBJ_LAYOUT_NAME(root_skiplist_manager), 
                  (unsigned long)SKIPLIST_MANAGER_POOL_SIZE, 0666); 
      if (pool == NULL) {
        printf("[ERROR] pmemobj_create\n");
        abort();
      }
      mapc = map_ctx_init(ops, pool);
      if (!mapc) {
        pmemobj_close(pool);
        printf("[ERROR] map_ctx_init\n");
        abort();
      }		
      struct hashmap_args args; // empty
      /* 
      * 1) Get root and make root ptr.
      *    skiplists' PMEMoid is oid of overall skiplists
      */
      root = POBJ_ROOT(pool, struct root_skiplist_manager);
      struct root_skiplist_manager *root_ptr = D_RW(root);
      /*
      * 2) Initialize root structure
      *    It includes malloc overall skiplists
      */
      root_ptr->count = SKIPLIST_MANAGER_LIST_SIZE;
      // printf("%d\n", root_ptr->count);
      TX_BEGIN(pool) {
        root_ptr->skiplists = pmemobj_tx_zalloc(
                        sizeof(root_skiplist_map) * SKIPLIST_MANAGER_LIST_SIZE, 
                        SKIPLIST_MAP_TYPE_OFFSET + 1000);
      } TX_ONABORT {
        printf("[ERROR] tx_abort on allocating skiplists\n");
        abort();
      } TX_END
      /* 
      * 3) map_create for all indices
      */
      skiplists = (struct root_skiplist_map *)pmemobj_direct(root_ptr->skiplists);
  		map = (TOID(struct map) *) malloc(
                        sizeof(TOID(struct map)) * SKIPLIST_MANAGER_LIST_SIZE);
      /* create */
      for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
        int res = map_create(mapc, &(skiplists[i].map), &args); 
        if (res) printf("[CREATE ERROR %d] %d\n",i ,res);
        else if (i==SKIPLIST_MANAGER_LIST_SIZE-1) printf("[CREATE SUCCESS %d]\n",i);	
        map[i] = skiplists[i].map;
      }
    } else {
      pool = pmemobj_open(SKIPLIST_MANAGER_PATH, 
                                      POBJ_LAYOUT_NAME(root_skiplist_manager));
      if (pool == NULL) {
        printf("[ERROR] pmemobj_create\n");
        abort();
		  }
      mapc = map_ctx_init(ops, pool);
      if (!mapc) {
        pmemobj_close(pool);
        printf("[ERROR] map_ctx_init\n");
        abort();
      }
      root = POBJ_ROOT(pool, struct root_skiplist_manager);
      struct root_skiplist_manager *root_ptr = D_RW(root);
      skiplists = (struct root_skiplist_map *)pmemobj_direct(root_ptr->skiplists);
      map = (TOID(struct map) *) malloc(
                        sizeof(TOID(struct map)) * SKIPLIST_MANAGER_LIST_SIZE);

      for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
				map[i] = skiplists[i].map;
      }
    }
  }
  
  PMEMobjpool* PmemSkiplist::GetPool() {
    return pool;
  };

  void PmemSkiplist::Insert(int index, char *key, char *value) {
    int res = map_insert(mapc, map[index], key, value);
    if(res) { fprintf(stderr, "[ERROR] insert %d\n", index);  abort();} 
    // else if (!res) printf("insert %d] success\n", index);
  }

} // namespace leveldb 