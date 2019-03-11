/*
 * [190220][JH][Tiered LSM-tree]
 * LAYOUT header file
 */

#ifndef LAYOUT_H
#define LAYOUT_H

#include <libpmemobj.h>
// #include "pmem/ds/skiplist_map.h"
/* Single skiplist */
#define SKIPLIST_PATH "/home/hwan/pmem_dir/skiplist"
#define SKIPLIST_POOL_SIZE 30 * (1 << 20) // temp setting

/* Multiple skiplists controlled by manager */
#define SKIPLIST_MANAGER_PATH "/home/hwan/pmem_dir/skiplist_manager"
#define SKIPLIST_MANAGER_PATH_0 "/home/hwan/pmem_dir/skiplist_manager_0"
#define SKIPLIST_MANAGER_PATH_1 "/home/hwan/pmem_dir/skiplist_manager_1"
#define SKIPLIST_MANAGER_PATH_2 "/home/hwan/pmem_dir/skiplist_manager_2"
#define SKIPLIST_MANAGER_PATH_3 "/home/hwan/pmem_dir/skiplist_manager_3"
#define SKIPLIST_MANAGER_PATH_4 "/home/hwan/pmem_dir/skiplist_manager_4"
#define SKIPLIST_MANAGER_PATH_5 "/home/hwan/pmem_dir/skiplist_manager_5"
#define SKIPLIST_MANAGER_PATH_6 "/home/hwan/pmem_dir/skiplist_manager_6"
#define SKIPLIST_MANAGER_PATH_7 "/home/hwan/pmem_dir/skiplist_manager_7"
#define SKIPLIST_MANAGER_PATH_8 "/home/hwan/pmem_dir/skiplist_manager_8"
#define SKIPLIST_MANAGER_PATH_9 "/home/hwan/pmem_dir/skiplist_manager_9"
/* 
 * FIXME: Seek proper pool size
 * 1.0 : 75-23961
 * 1.95 : 148-12508
 * 3 : 228-23674
 */
// #define SKIPLIST_MANAGER_POOL_SIZE 300 * (1 << 20)
#define SKIPLIST_MANAGER_POOL_SIZE 2 * (1 << 30)
#define SKIPLIST_MANAGER_LIST_SIZE 100
// #define SKIPLIST_MANAGER_LIST_SIZE 10
#define SKIPLIST_BULK_INSERT_NUM 32077
// #define SKIPLIST_BULK_INSERT_NUM 10
#define NUM_OF_SKIPLIST_MANAGER 10

#define SKIPLIST_OFFSET_PATH "/home/hwan/pmem_dir/skiplist_offset"
#define SKIPLIST_OFFSET_SIZE 100 * (1<< 20)
#define SKIPLIST_OFFSET_LAYOUT "skiplist_offset"
#define NUM_OF_SKIPLIST_OFFSET \
(SKIPLIST_MANAGER_LIST_SIZE * NUM_OF_SKIPLIST_MANAGER)

namespace levedb {

// POBJ_LAYOUT_ROOT(root_skiplist_map, struct skiplist_map_node);

// struct root_skiplist_map {
//  	TOID(struct skiplist_map_node) map;
// };
// TOID_DECLARE_ROOT(struct root_skiplist_map);

// typedef struct Root_skiplist_map {
//  	char buf[3000];
// } root_skiplist_map;

// TOID_DECLARE_ROOT(struct root_skiplist_map);

}
#endif