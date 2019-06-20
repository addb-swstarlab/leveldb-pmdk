/*
 * [190220][JH][Tiered LSM-tree]
 * LAYOUT header file
 */

#ifndef LAYOUT_H
#define LAYOUT_H

#include <libpmemobj.h>

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
#define SKIPLIST_MANAGER_PATH_10 "/home/hwan/pmem_dir/skiplist_manager_10"
#define SKIPLIST_MANAGER_PATH_11 "/home/hwan/pmem_dir/skiplist_manager_11"

// #define SKIPLIST_MANAGER_POOL_SIZE 300 * (1 << 20)
#define SKIPLIST_MANAGER_POOL_SIZE 2 * (1 << 30)
/* 
 * EVALUATION 1: write_buffer_size = 4MB (default)
 * value 100bytes - 28300 (28221) MAX_LIST_SIZE 290
 * value 200bytes - 16400 (16353) 500
 * value 400bytes - 9200  (9179) 900
 * value 800bytes - 4100  (4089) 2000
 * value 1600bytes - 2560 (2536) 3200
 */
/* 
 * EVALUATION 2: write_buffer_size = 64MB
 * value 1KB - 62270 (62266) MAX_LIST_SIZE 130
 * value 2KB - 31935 (31930) MAX_LIST_SIZE 255
 * value 4KB - 16180 (16171) 510
 * value 8KB - 8140 (8139) 1000
 * value 16KB - 4090 (4083) 2000
 */
// #define MAX_SKIPLIST_NODE_SIZE 28300 // Compaction Output file max
// #define SKIPLIST_BULK_INSERT_NUM 10
// #define SKIPLIST_MANAGER_LIST_SIZE 290
// for YCSB
#define MAX_SKIPLIST_NODE_SIZE 58830 // Compaction Output file max
#define SKIPLIST_MANAGER_LIST_SIZE 140

#define NUM_OF_SKIPLIST_MANAGER 10
// #define NUM_OF_SKIPLIST_MANAGER 12

#define BUFFER_PATH "/home/hwan/pmem_dir/pmem_buffer"
#define BUFFER_PATH_0 "/home/hwan/pmem_dir/pmem_buffer_0"
#define BUFFER_PATH_1 "/home/hwan/pmem_dir/pmem_buffer_1"
#define BUFFER_PATH_2 "/home/hwan/pmem_dir/pmem_buffer_2"
#define BUFFER_PATH_3 "/home/hwan/pmem_dir/pmem_buffer_3"
#define BUFFER_PATH_4 "/home/hwan/pmem_dir/pmem_buffer_4"
#define BUFFER_PATH_5 "/home/hwan/pmem_dir/pmem_buffer_5"
#define BUFFER_PATH_6 "/home/hwan/pmem_dir/pmem_buffer_6"
#define BUFFER_PATH_7 "/home/hwan/pmem_dir/pmem_buffer_7"
#define BUFFER_PATH_8 "/home/hwan/pmem_dir/pmem_buffer_8"
#define BUFFER_PATH_9 "/home/hwan/pmem_dir/pmem_buffer_9"
#define BUFFER_PATH_10 "/home/hwan/pmem_dir/pmem_buffer_10"
#define BUFFER_PATH_11 "/home/hwan/pmem_dir/pmem_buffer_11"
#define BUFFER_PATH_12 "/home/hwan/pmem_dir/pmem_buffer_12"
#define BUFFER_PATH_13 "/home/hwan/pmem_dir/pmem_buffer_13"
#define BUFFER_PATH_14 "/home/hwan/pmem_dir/pmem_buffer_14"

// #define BUFFER_POOL_SIZE 2.5 * (1 << 30)
// #define BUFFER_POOL_SIZE 2684354560
#define BUFFER_POOL_SIZE 2 * (1 << 30)
#define NUM_OF_BUFFER 10
// #define NUM_OF_BUFFER 13
#define NUM_OF_CONTENTS 350
#define EACH_CONTENT_SIZE 4 << 20 // FIXME: 4MB
#define MAX_CONTENTS_SIZE (NUM_OF_CONTENTS * EACH_CONTENT_SIZE)

#define FREE_LIST_WARNING_BOUNDARY 10

// PROGRESS: Hashmap
#define HASHMAP_PATH "/home/hwan/pmem_dir/pmem_hashmap"
#define HASHMAP_PATH_0 "/home/hwan/pmem_dir/pmem_hashmap_0"
#define HASHMAP_PATH_1 "/home/hwan/pmem_dir/pmem_hashmap_1"
#define HASHMAP_PATH_2 "/home/hwan/pmem_dir/pmem_hashmap_2"
#define HASHMAP_PATH_3 "/home/hwan/pmem_dir/pmem_hashmap_3"
#define HASHMAP_PATH_4 "/home/hwan/pmem_dir/pmem_hashmap_4"
#define HASHMAP_PATH_5 "/home/hwan/pmem_dir/pmem_hashmap_5"
#define HASHMAP_PATH_6 "/home/hwan/pmem_dir/pmem_hashmap_6"
#define HASHMAP_PATH_7 "/home/hwan/pmem_dir/pmem_hashmap_7"
#define HASHMAP_PATH_8 "/home/hwan/pmem_dir/pmem_hashmap_8"
#define HASHMAP_PATH_9 "/home/hwan/pmem_dir/pmem_hashmap_9"

#define HASHMAP_POOL_SIZE 2 * (1 << 20)
#define HASHMAP_LIST_SIZE 100
#define NUM_OF_HASHMAP 10

namespace levedb {

}
#endif