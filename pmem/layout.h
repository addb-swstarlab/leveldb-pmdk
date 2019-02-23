/*
 *
 */

// #define POOL1 "PMAP_LAYOUT"
// #define PMAP_PATH "/home/hwan/pmem_dir/pmap"
// // #define PMAP_SIZE 1.95 * (1 << 30) // About 1.95GB
// #define PMAP_SIZE 30 * (1 << 20) // About 1.95GB
// #define NUM_OF_PMAP 10
// #define BUF_SIZE 3000

#ifndef LAYOUT_H
#define LAYOUT_H

#include <libpmemobj.h>
#include "pmem/ds/skiplist_map.h"
#define MAP_PATH "/home/hwan/pmem_dir/skiplist_map"
#define MAP_SIZE 30 * (1 << 20) // temp setting

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