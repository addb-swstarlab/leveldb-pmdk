/*
 * Copyright 2016, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * skiplist.c -- no pmembuffer Skiplist implementation
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "pmem/ds/skiplist.h"

#include <chrono>
#include <iostream>

#define SKIPLIST_LEVELS_NUM 12
#define NULL_NODE TOID_NULL(struct skiplist_map_node)

namespace leveldb {

struct skiplist_map_entry {
	PMEMoid key;
	uint8_t key_len;
	PMEMoid value;
	uint8_t value_len; // need to find
	// TEST:
	void* key_ptr;
	void* value_ptr;
};

struct skiplist_map_node {
	TOID(struct skiplist_map_node) next[SKIPLIST_LEVELS_NUM];
	struct skiplist_map_entry entry;
};

/* 
 * LAST_NODE for find insert-position(next) 
 * It's common structure
 */
struct store_last_node {
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];
};
struct store_last_node last_node;
// TOID(struct skiplist_map_node) current_node[NUM_OF_PRE_ALLOC_NODE];
int common_constant;

/*
 * skiplist_map_clear -- removes all elements from the map
 */
int
skiplist_map_clear(PMEMobjpool *pop, TOID(struct skiplist_map_node) map)
{
	/*
	while (!TOID_EQUALS(D_RO(map)->next[0], NULL_NODE)) {
		TOID(struct skiplist_map_node) next = D_RO(map)->next[0];

		uint8_t key_len = D_RO(next)->entry.key_len+STRING_PADDING;
		char *buf = (char *)malloc(key_len);
		void *ptr = pmemobj_direct(D_RO(next)->entry.key);
		pmemobj_memcpy_persist(pop, buf, ptr, key_len);
		skiplist_map_remove_free(pop, map, buf);
		free(buf);
	}
	*/
	TOID(struct skiplist_map_node) next = D_RO(map)->next[0];
	while (!TOID_EQUALS(next, NULL_NODE)) {
		D_RW(next)->entry.key_len = 0;
		D_RW(next)->entry.value_len = 0;
		D_RW(next)->entry.key_ptr = nullptr;
		D_RW(next)->entry.value_ptr = nullptr;
		next = D_RO(next)->next[0];
	}
	return 0;
}

/*
 * skiplist_map_destroy -- cleanups and frees skiplist instance
 */
int
skiplist_map_destroy(PMEMobjpool *pop, TOID(struct skiplist_map_node) *map)
{
	int ret = 0;
	TX_BEGIN(pop) {
		// printf("[DEBUG] 1\n");
		skiplist_map_clear(pop, *map);
		// printf("[DEBUG] 2\n");
		/* 
		 * TODO: [190222][JH], Need to fix this function call...
		 * Since this occurs error, it goes to TX_ONABORT state
		 */
		// pmemobj_tx_add_range_direct(map, sizeof(*map));
		// printf("[DEBUG] 3\n");
		TX_FREE(*map);
		*map = TOID_NULL(struct skiplist_map_node);
	} TX_ONABORT {
		ret = 1;
	} TX_END
	return ret;
}
/*
 * skiplist_map_insert_node -- (internal) adds new node in selected place
 * Called by insert. Start from L1. (Additional linking)
 */
static void
skiplist_map_insert_node(TOID(struct skiplist_map_node) new_node,
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM])
{
	// std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	unsigned current_level = 0;
	// FIXME: modify random addtion logic
	common_constant++; // Make distribution logic
	do {
		if (current_level == 0 ||
				(current_level == 1 && common_constant % LEVEL_1_POINT == 0 ) ||
				(current_level == 2 && common_constant % LEVEL_2_POINT == 0 ) ||
				(current_level == 3 && common_constant % LEVEL_3_POINT == 0 ) ||
				(current_level == 4 && common_constant % LEVEL_4_POINT == 0 ) ||
				(current_level == 5 && common_constant % LEVEL_5_POINT == 0 ) ||
				(current_level == 6 && common_constant % LEVEL_6_POINT == 0 ) ||
				(current_level == 7 && common_constant % LEVEL_7_POINT == 0 ) ||
				(current_level == 8 && common_constant % LEVEL_8_POINT == 0 ) ||
				(current_level == 9 && common_constant % LEVEL_9_POINT == 0 ) ||
				(current_level == 10 && common_constant % LEVEL_10_POINT == 0 ) ||
				(current_level == 11 && common_constant % LEVEL_11_POINT == 0 )
				) {
			TX_ADD_FIELD(path[current_level], next[current_level]);
			D_RW(new_node)->next[current_level] =
				D_RO(path[current_level])->next[current_level];
			D_RW(path[current_level])->next[current_level] = new_node;
		}
	// } while (++current_level < SKIPLIST_LEVELS_NUM && rand() % 2 == 0);
	} while (++current_level < SKIPLIST_LEVELS_NUM);

	// std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();
	// std::cout << "insert-node = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";
}

/*
 * skiplist_map_map_find -- (internal) returns path to searched node, or if
 * node doesn't exist, it will return path to place where key should be.
 */
static void
skiplist_map_find(PMEMobjpool *pop, char *key, 
	TOID(struct skiplist_map_node) map, TOID(struct skiplist_map_node) *path)
{
	/*
	int current_level;
	TOID(struct skiplist_map_node) active = map;
	for (current_level = SKIPLIST_LEVELS_NUM - 1;
			current_level >= 0; current_level--) {
		TOID(struct skiplist_map_node) next = D_RO(active)->next[current_level];
		for ( void *ptr ;
				!TOID_EQUALS(next, NULL_NODE);
				next = D_RO(active)->next[current_level]) {
			// Only compare key
			uint8_t key_len = D_RO(next)->entry.key_len-NUM_OF_TAG_BYTES;
			ptr = pmemobj_direct(D_RO(next)->entry.key);
			// printf("[DEBUG %d] key:'%s' ptr:'%s'\n", current_level, key, (char *)ptr);
			if (memcmp(key, ptr, key_len) <= 0) {
			// if (strcmp(key, (char *)ptr) <= 0)  { // ascending order
			// printf("[Break DEBUG %d] key:'%s' ptr:'%s'\n", current_level, key, (char *)ptr);
				break;
			}
			active = next;
		}
		path[current_level] = active;
	}
	*/
	// [Original] Compare all characters
	// /*
	int current_level;
	TOID(struct skiplist_map_node) active = map;
	for (current_level = SKIPLIST_LEVELS_NUM - 1;
			current_level >= 0; current_level--) {
		TOID(struct skiplist_map_node) next = D_RO(active)->next[current_level];
		for ( void *ptr ;
				!TOID_EQUALS(next, NULL_NODE);
				next = D_RO(active)->next[current_level]) {
			uint8_t key_len = D_RO(next)->entry.key_len+STRING_PADDING;
			ptr = pmemobj_direct(D_RO(next)->entry.key);
			// printf("[DEBUG %d] key:'%s' ptr:'%s'\n", current_level, key, (char *)ptr);
			if (strcmp(key, (char *)ptr) <= 0)  { // ascending order
			// if (strcmp(key, (char *)ptr) >= 0)  { // descending order
			// printf("[Break DEBUG %d] key:'%s' ptr:'%s'\n", current_level, key, (char *)ptr);
				break;
			}
			active = next;
		}
		path[current_level] = active;
	}
	// */
}
/*
 * skiplist_map_insert_find -- (internal) returns path to last node, or if
 * node doesn't exist, it will return path to place where key should be.
 */
static void
skiplist_map_insert_find(PMEMobjpool *pop,
	TOID(struct skiplist_map_node) map, TOID(struct skiplist_map_node) *path)
{
	// std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	int current_level;
	TOID(struct skiplist_map_node) active = map;
	for (current_level = SKIPLIST_LEVELS_NUM - 1;
			current_level >= 0; current_level--) {
		TOID(struct skiplist_map_node) next = D_RO(active)->next[current_level];
		if (TOID_EQUALS(next, NULL_NODE)) {
			path[current_level] = active;
			last_node.path[current_level] = active;
		} else {
			active = last_node.path[current_level];
			next = D_RO(active)->next[current_level];
			if (TOID_EQUALS(next, NULL_NODE)) {
				path[current_level] = active;
				last_node.path[current_level] = active;
			} else {
				path[current_level] = next;
				last_node.path[current_level] = next;
			}
		}
	}
	// std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();
	// std::cout << "find = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<" / ";
}
/*
 * skiplist_map_insert -- inserts a new key-value pair into the map
 * PROGRESS:
 */
int
skiplist_map_insert(PMEMobjpool *pop, 
	TOID(struct skiplist_map_node) map, 
	TOID(struct skiplist_map_node) *current_node,
	char *key, char *value, int key_len, int value_len, int index)
	// char *key, PMEMoid value)
{
	// std::chrono::steady_clock::time_point begin, end;
	int ret = 0;
	/* Get from L0 */
		// begin = std::chrono::steady_clock::now();
	TOID(struct skiplist_map_node) new_node = D_RW(*current_node)->next[0];
	if (TOID_IS_NULL(new_node) || TOID_EQUALS(new_node, NULL_NODE)) {
		printf("[ERROR][Skiplist][insertion] Out of bound \n");
	}
	*current_node = new_node;
	// 	end= std::chrono::steady_clock::now();
	// std::cout << "insert0 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";
	// TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];
	// TX_BEGIN(pop) {
		// begin = std::chrono::steady_clock::now();
		void *key_ptr = pmemobj_direct(D_RW(new_node)->entry.key);
	// 	end= std::chrono::steady_clock::now();
	// std::cout << "insert1 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<" / ";
	// 	begin = std::chrono::steady_clock::now();
		pmemobj_memcpy_persist(pop, (char *)key_ptr, (char *)key, key_len);
		// printf("len: %d\n", key_len);
	// 	end= std::chrono::steady_clock::now();
	// std::cout << "insert2 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<" / ";
	// 	begin = std::chrono::steady_clock::now();
		D_RW(new_node)->entry.key_len = (uint8_t)key_len; 
	// 	end= std::chrono::steady_clock::now();
	// std::cout << "insert3 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";

		// begin = std::chrono::steady_clock::now();
		void *value_ptr = pmemobj_direct(D_RW(new_node)->entry.value);
	// 	end= std::chrono::steady_clock::now();
	// std::cout << "insert4 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<" / ";
	// 	begin = std::chrono::steady_clock::now();
		pmemobj_memcpy_persist(pop, (char *)value_ptr, (char *)value, value_len);
	// 	end= std::chrono::steady_clock::now();
	// std::cout << "insert5 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<" / ";
	// 	begin = std::chrono::steady_clock::now();
		D_RW(new_node)->entry.value_len = (uint8_t)value_len; 
		// end= std::chrono::steady_clock::now();
		// std::cout << "insert6 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";
		// printf("keylen:%d, valuelen:%d\n",D_RW(new_node)->entry.key_len, D_RW(new_node)->entry.value_len);

		// TEST:
		// D_RW(new_node)->entry.key_ptr = nullptr;
		// D_RW(new_node)->entry.value_ptr = nullptr;
		/* Insert new_node into L1 ~ L3 */
		// skiplist_map_insert_find(pop, map, path);
		// skiplist_map_insert_node(new_node, path);

	// } TX_ONABORT {
	// 	ret = 1;
	// } TX_END
	return ret;
}
/*
 * PROGRESS:
 *  skiplist_map_insert_by_oid -- inserts a specifics node into the map by kv oid
 */
int
skiplist_map_insert_by_oid(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	TOID(struct skiplist_map_node) *current_node, 
	PMEMoid *key_oid, PMEMoid *value_oid, int key_len, int value_len, int index)
	// char *key, PMEMoid value)
{
	// std::chrono::steady_clock::time_point begin, end;
	int ret = 0;
	/* Get from L0 */
	// begin = std::chrono::steady_clock::now();
	TOID(struct skiplist_map_node) new_node = D_RW(*current_node)->next[0];
	// TOID(struct skiplist_map_node) new_node = current_node[index];
	if (TOID_IS_NULL(new_node) || TOID_EQUALS(new_node, NULL_NODE)) {
		printf("[ERROR][Skiplist][insertionByOID] Out of bound \n");
	}
	// new_node.oid = *key_oid;
	// pmemobj_free(&D_RW(new_node)->entry.key);
	// pmemobj_free(&D_RW(new_node)->entry.value);
	D_RW(new_node)->entry.key = *key_oid;
	D_RW(new_node)->entry.key_len = (uint8_t)key_len;
	D_RW(new_node)->entry.value = *value_oid;
	D_RW(new_node)->entry.value_len = (uint8_t)value_len; 

	*current_node = new_node;
	// end= std::chrono::steady_clock::now();
	// std::cout << "insert by oid = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";

	// } TX_ONABORT {
	// 	ret = 1;
	// } TX_END
	return ret;
}
/*
 * TEST:
 *  skiplist_map_insert_by_ptr -- inserts a specifics node into the map by kv ptr
 */
int
skiplist_map_insert_by_ptr(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	TOID(struct skiplist_map_node) *current_node, 
	// PMEMoid *key_oid, PMEMoid *value_oid, int key_len, int value_len, int index)
	void* key_ptr, void* value_ptr, int key_len, int value_len, int index)
{
	// std::chrono::steady_clock::time_point begin, end;
	int ret = 0;
	/* Get from L0 */
	// begin = std::chrono::steady_clock::now();
	TOID(struct skiplist_map_node) new_node = D_RW(*current_node)->next[0];
	// TOID(struct skiplist_map_node) new_node = current_node[index];
	if (TOID_IS_NULL(new_node) || TOID_EQUALS(new_node, NULL_NODE)) {
		printf("[ERROR][Skiplist][insertionByPTR] Out of bound \n");
	}
	// new_node.oid = *key_oid;
	// pmemobj_free(&D_RW(new_node)->entry.key);
	// pmemobj_free(&D_RW(new_node)->entry.value);
	
	// D_RW(new_node)->entry.key = *key_oid;
	// D_RW(new_node)->entry.value = *value_oid;
	// D_RW(new_node)->entry.key_ptr = pmemobj_direct(D_RO(new_node)->entry.key);
	// D_RW(new_node)->entry.value_ptr = pmemobj_direct(D_RO(new_node)->entry.value);

	// D_RW(new_node)->entry.key_ptr = pmemobj_direct(*key_oid);
	D_RW(new_node)->entry.key_ptr = key_ptr;
	D_RW(new_node)->entry.key_len = (uint8_t)key_len;
	// D_RW(new_node)->entry.value_ptr = pmemobj_direct(*value_oid);
	D_RW(new_node)->entry.value_ptr = value_ptr;
	D_RW(new_node)->entry.value_len = (uint8_t)value_len; 
	// printf("Insert_by_ptr [key] '%s'\n", D_RO(new_node)->entry.key_ptr);
	// printf("Insert_by_ptr [value] '%s'\n", D_RO(new_node)->entry.value_ptr);

	*current_node = new_node;
	// end= std::chrono::steady_clock::now();
	// std::cout << "insert by oid = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";

	// } TX_ONABORT {
	// 	ret = 1;
	// } TX_END
	return ret;
}

/*
 * PROGRESS:
 *  skiplist_map_insert_null_node -- inserts null node at last 
 */
int
skiplist_map_insert_null_node(PMEMobjpool *pop, 
	TOID(struct skiplist_map_node) map,
	TOID(struct skiplist_map_node) *current_node, int index)
	// char *key, PMEMoid value)
{
	// std::chrono::steady_clock::time_point begin, end;
	int ret = 0;
	/* Get from L0 */
	// begin = std::chrono::steady_clock::now();
	// TOID(struct skiplist_map_node) new_node = D_RW(current_node[index])->next[0];
	TOID(struct skiplist_map_node) null_node = NULL_NODE;
	// TOID(struct skiplist_map_node) new_node = current_node[index];
	// if (TOID_IS_NULL(new_node) || TOID_EQUALS(new_node, NULL_NODE)) {
		// new_node.oid = *oid;
		D_RW(*current_node)->next[0] = null_node;
		*current_node = NULL_NODE;
	// }
	// 1) exchange oid

	// 2) adjust linking-relationship

	// end= std::chrono::steady_clock::now();
	// std::cout << "insert by oid = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";

		// skiplist_map_insert_find(pop, map, path);
		// skiplist_map_insert_node(new_node, path);

	// } TX_ONABORT {
	// 	ret = 1;
	// } TX_END
	return ret;
}

/*
 * skiplist_map_create_insert -- (internal) inserts an empty key-value pair 
 * into the map. Just pre-allocate new-node and key&value area
 */
int
skiplist_map_create_insert(PMEMobjpool *pop, TOID(struct skiplist_map_node) map)
	// char *key, PMEMoid value)
{
	int ret = 0;
	TOID(struct skiplist_map_node) new_node;
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];

	// std::chrono::steady_clock::time_point begin, end;
	TX_BEGIN(pop) {
		// printf("111\n");
		new_node = TX_ZNEW(struct skiplist_map_node);
		// printf("222\n");
		D_RW(new_node)->entry.key = pmemobj_tx_zalloc(PRE_ALLOC_KEY_SIZE, 500); // temp, string:= 500
		// printf("333\n");
		D_RW(new_node)->entry.value = pmemobj_tx_zalloc(PRE_ALLOC_VALUE_SIZE, 600); // temp, string:= 500

		// TEST:
		D_RW(new_node)->entry.key_ptr = nullptr;
		D_RW(new_node)->entry.value_ptr = nullptr;
		// printf("444\n");
		// skiplist_map_create_find(pop, map, path);
		skiplist_map_insert_find(pop, map, path);
		// printf("555\n");
		// skiplist_map_create_insert_node(new_node, path);
		skiplist_map_insert_node(new_node, path);
		// printf("666\n");
	} TX_ONABORT {
		ret = 1;
	} TX_END
	return ret;
}
/*
 * skiplist_map_create -- allocates a new skiplist instance
 * PROGRESS:
 */
int
skiplist_map_create(PMEMobjpool *pop, TOID(struct skiplist_map_node) *map,
	TOID(struct skiplist_map_node) current_node,
	int index, void *arg)
{
	int ret = 0;
	// NOTE: initialize common_constant
	common_constant = 0;
	TX_BEGIN(pop) {
		// printf("[DEBUG1]");
		/* Head node */
		// printf(" - [DEBUG2]");
		/* Pre-allcate estimated-nodelength */
		if ((char *)arg != nullptr) {
			pmemobj_tx_add_range_direct(map, sizeof(*map));
			*map = TX_ZNEW(struct skiplist_map_node);
			for (int i=0; i< NUM_OF_PRE_ALLOC_NODE; i++) {
				// printf("i: %d\n", i);
				skiplist_map_create_insert(pop, *map);
			}
		}
		/* Reset current_node */
		// current_node = *map;

		// printf(" - [DEBUG3] \n");
	} TX_ONABORT {
		ret = 1;
	} TX_END
	return ret;
}

/*
 * skiplist_map_remove_node -- (internal) removes selected node
 */
static void
skiplist_map_remove_node(
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM])
{
	TOID(struct skiplist_map_node) to_remove = D_RO(path[0])->next[0];
	int i;
	for (i = 0; i < SKIPLIST_LEVELS_NUM; i++) {
		if (TOID_EQUALS(D_RO(path[i])->next[i], to_remove)) {
			TX_ADD_FIELD(path[i], next[i]);
			D_RW(path[i])->next[i] = D_RO(to_remove)->next[i];
		}
	}
}

/*
 * skiplist_map_remove_free -- removes and frees an object from the list
 */
int
skiplist_map_remove_free(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	char *key)
{
	int ret = 0;
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];
	TOID(struct skiplist_map_node) to_remove;
	TX_BEGIN(pop) {
		skiplist_map_find(pop, key, map, path);
		to_remove = D_RO(path[0])->next[0];
		if (!TOID_EQUALS(to_remove, NULL_NODE)) {
			uint8_t key_len = D_RO(to_remove)->entry.key_len+STRING_PADDING;
			char *buf = (char *)malloc(key_len);
			void *ptr = pmemobj_direct(D_RO(to_remove)->entry.key);
			pmemobj_memcpy_persist(pop, buf, ptr, key_len);
	// printf("[REMOVE_FREE-DEBUG] key:'%s' ptr:'%s' buf:'%s'\n", key, (char *)ptr, buf);
			if (strcmp(buf, key) == 0) {
				pmemobj_tx_free(D_RW(to_remove)->entry.key);
				pmemobj_tx_free(D_RW(to_remove)->entry.value);
				skiplist_map_remove_node(path);
				ret = 1;
			} 
			free(buf);
		}
	} TX_ONABORT {
		// ret = 1;
	} TX_END

	return ret;
}

/*
 * skiplist_map_remove -- removes key-value pair from the map
 */
char*
skiplist_map_remove(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	char *key)
{
	char *ret = new char;
	strcpy(ret, "");
	// PMEMoid ret = OID_NULL;
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];
	TOID(struct skiplist_map_node) to_remove;
	TX_BEGIN(pop) {
		skiplist_map_find(pop, key, map, path);
		to_remove = D_RO(path[0])->next[0];
		if (!TOID_EQUALS(to_remove, NULL_NODE)) {
			uint8_t key_len = D_RO(to_remove)->entry.key_len+STRING_PADDING;
			char *buf = (char *)malloc(key_len);
			void *ptr = pmemobj_direct(D_RO(to_remove)->entry.key);
			pmemobj_memcpy_persist(pop, buf, ptr, key_len);
	// printf("[REMOVE-DEBUG] key:'%s' ptr:'%s' buf:'%s'\n", key, (char *)ptr, buf);
			if (strcmp(buf, key) == 0) {
				free(buf);
				uint8_t value_len = D_RO(to_remove)->entry.value_len+STRING_PADDING;
				buf = (char *)malloc(value_len);
				ptr = pmemobj_direct(D_RO(to_remove)->entry.value);
				pmemobj_memcpy_persist(pop, buf, ptr, value_len);
				strcpy(ret, buf);
				skiplist_map_remove_node(path);
			} 
			free(buf);
		}
	} TX_ONABORT {
		// ret = "";
	} TX_END

	return ret;
}
/*
 * skiplist_map_get_find -- (internal) returns path to searched node, or if
 * node doesn't exist, it will return path to place where key should be.
 */
static void
skiplist_map_get_find(PMEMobjpool *pop, char *key, 
	TOID(struct skiplist_map_node) map, TOID(struct skiplist_map_node) *path,
	int *find_res)
{
	// std::chrono::steady_clock::time_point begin, end;
	
		// begin = std::chrono::steady_clock::now();
	int current_level;
	TOID(struct skiplist_map_node) active = map;
	for (current_level = SKIPLIST_LEVELS_NUM - 1;
			current_level >= 0; current_level--) {

		// void *key_ptr = pmemobj_direct(D_RO(active)->entry.key);
		// printf("[Before DEBUG %d] key:'%s' ptr:'%s' \n", current_level, key, (char *)key_ptr);

		TOID(struct skiplist_map_node) next = D_RO(active)->next[current_level];
		for ( void *ptr ;
				!TOID_EQUALS(next, NULL_NODE);
				next = D_RO(active)->next[current_level]) {
			uint8_t key_len = D_RO(next)->entry.key_len;
			// NOTE: Only compare key
			// Avoid looping about empty&pre-allocated key
			if (key_len == 0) {
				if (current_level == 0) {
					path[current_level] = NULL_NODE;
				}
				break;
			} else if (key_len > NUM_OF_TAG_BYTES){
				key_len -= NUM_OF_TAG_BYTES;
			}
			// TEST: Check key_ptr & value_ptr
			int res_cmp;
			if (D_RO(next)->entry.key_ptr != nullptr &&
					D_RO(next)->entry.value_ptr != nullptr) {
				// printf("Here]]\n");
				ptr = D_RO(next)->entry.key_ptr;
				// res_cmp = memcmp(key, D_RO(next)->entry.key_ptr, key_len);
			} else {
				ptr = pmemobj_direct(D_RO(next)->entry.key);
			}
			res_cmp = memcmp(key, ptr, key_len);
			// printf("[DEBUG %d] key:'%s' ptr:'%s'\n", current_level, key, (char *)ptr);

			if (res_cmp < 0) {
			// printf("[Break DEBUG %d] key:'%s' ptr:'%s'\n", current_level, key, (char *)ptr);
				break;
			} 
			// Immediatly stop
			else if (res_cmp == 0) {
				// printf("Break1 '%s'\n", key);
				// printf("Break2 '%s'\n", (char *)ptr);
				*find_res = current_level;
				break;
			}
			active = next;
		}
		// Immediatly stop
		path[current_level] = active;
		if (*find_res >= 0) { 
			break;
		}
	}
			// end= std::chrono::steady_clock::now();
			// std::cout << "get_find_time = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";
}
/*
 * skiplist_map_get_prev_find -- (internal) returns path to searched node, or if
 * node doesn't exist, it will return path to place where key should be.
 */
static void
skiplist_map_get_prev_find(PMEMobjpool *pop, char *key, 
	TOID(struct skiplist_map_node) map, TOID(struct skiplist_map_node) *path,
	int *find_res)
{
	int current_level;
	TOID(struct skiplist_map_node) active = map;
	for (current_level = SKIPLIST_LEVELS_NUM - 1;
			current_level >= 0; current_level--) {

		// void *key_ptr = pmemobj_direct(D_RO(active)->entry.key);
		// printf("[Before DEBUG %d] key:'%s' ptr:'%s'\n", current_level, key, (char *)key_ptr);

		TOID(struct skiplist_map_node) next = D_RO(active)->next[current_level];
		for ( void *ptr ;
				!TOID_EQUALS(next, NULL_NODE);
				next = D_RO(active)->next[current_level]) {
			// NOTE: Key-matching exactly
			uint8_t key_len = D_RO(next)->entry.key_len;
			// Avoid looping about empty&pre-allocated key
			// if (strlen((char *)ptr) == 0) {
			if (key_len == 0) {
				break;
			}
			ptr = pmemobj_direct(D_RO(next)->entry.key);
			// printf("[DEBUG %d] key:'%s' ptr:'%s'\n", current_level, key, (char *)ptr);
			int res_cmp = memcmp(key, ptr, key_len);
			if (res_cmp < 0) {
			// printf("[Break DEBUG %d] key:'%s' ptr:'%s'\n", current_level, key, (char *)ptr);
				break;
			} 
			// Immediatly stop
			else if (res_cmp == 0) {
				*find_res = current_level;
				break;
			}
			active = next;
		}
		// Immediatly stop
		path[current_level] = active;
		if (*find_res >= 0) { 
			break;
		}
	}
}
/*
 * skiplist_map_get_last_find -- (internal) returns path to searched node, or if
 * node doesn't exist, it will return path to place where key should be.
 */
static void
skiplist_map_get_last_find(PMEMobjpool *pop, 
	TOID(struct skiplist_map_node) map, TOID(struct skiplist_map_node) *path)
{
	int current_level;
	TOID(struct skiplist_map_node) active = map;
	for (current_level = SKIPLIST_LEVELS_NUM - 1;
			current_level >= 0; current_level--) {
		TOID(struct skiplist_map_node) next = D_RO(active)->next[current_level];
		for ( void *ptr ;
				!TOID_EQUALS(next, NULL_NODE);
				next = D_RO(active)->next[current_level]) {
			// NOTE: Check only Key-length
			uint8_t key_len = D_RO(next)->entry.key_len;
			// uint8_t value_len = D_RO(next)->entry.value_len;
			// Seek first empty node in each level
			if (key_len == 0) break;
			// if (key_len == 0 & value_len == 0) break;
			active = next;
		}
		// Immediatly stop
		path[current_level] = active;
	}
}

/*
 * skiplist_map_get -- searches for a value of the key
 */
char*
skiplist_map_get(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	char *key)
{	
	// std::chrono::steady_clock::time_point begin, end;
	// begin = std::chrono::steady_clock::now();

	int exactly_found_level = -1;
	char *res;
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM], found;
	// end= std::chrono::steady_clock::now();
	// std::cout << "get1 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<" / ";
	// begin = std::chrono::steady_clock::now();

	// FIXME: Implement only-key comparison
	skiplist_map_get_find(pop, key, map, path, &exactly_found_level);
	// end= std::chrono::steady_clock::now();
	// std::cout << "get2 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<" / ";
	// begin = std::chrono::steady_clock::now();
	if (exactly_found_level != -1) {
		// printf("1\n");
		// printf("find_res: %d\n",find_res );
	// end= std::chrono::steady_clock::now();
	// std::cout << "get3 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<" / ";
	// begin = std::chrono::steady_clock::now();
		found = D_RO(path[exactly_found_level])->next[exactly_found_level];
		uint8_t value_len = D_RO(found)->entry.value_len+STRING_PADDING;
		void *key_ptr = pmemobj_direct(D_RO(found)->entry.key);
		void *ptr = pmemobj_direct(D_RO(found)->entry.value);
		res = new char[value_len];
		memcpy(res, ptr, value_len);
		// printf("Get key %d: '%s'\n", exactly_found_level, key_ptr);
	// end= std::chrono::steady_clock::now();
	// std::cout << "get4 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";
	} else {
		// printf("2\n");
		found = D_RO(path[0])->next[0];
		if (!TOID_EQUALS(found, NULL_NODE)) {
		// printf("3\n");
			uint8_t key_len = D_RO(found)->entry.key_len+STRING_PADDING-NUM_OF_TAG_BYTES;
			void *ptr = pmemobj_direct(D_RO(found)->entry.key);
		// printf("4\n");
		// printf("[GET-DEBUG] key:'%s' ptr:'%s' buf:'%s'\n", key, (char *)ptr, buf);
			if (memcmp(ptr, key, key_len) == 0) {
		// printf("6\n");
				uint8_t value_len = D_RO(found)->entry.value_len+STRING_PADDING;
				void *value_ptr = pmemobj_direct(D_RO(found)->entry.value);
				res = new char[value_len];
				memcpy(res, value_ptr, value_len);
				// printf("Get key %d : '%s'-'%s'\n", exactly_found_level, key, ptr);
			} else {
				printf("[Get-ERROR] Cannot seek key '%s' \n", key);
			}
		} else {
		// 	found = path[0];
		// 	uint8_t key_len = D_RO(found)->entry.key_len+STRING_PADDING-NUM_OF_TAG_BYTES;
		// 	void *ptr = pmemobj_direct(D_RO(found)->entry.key);
		// // printf("[GET-DEBUG] key:'%s' ptr:'%s' buf:'%s'\n", key, (char *)ptr, buf);
		// 	if (memcmp(ptr, key, key_len-NUM_OF_TAG_BYTES) == 0) {
		// 		uint8_t value_len = D_RO(found)->entry.value_len+STRING_PADDING;
		// 		ptr = pmemobj_direct(D_RO(found)->entry.value);
		// 		res = new char[value_len];
		// 		memcpy(res, ptr, value_len);
		// 	} else {
		// 	} 
			res = new char;
			strcpy(res, "");
			printf("[Get-ERROR] Reach first empty node '%s'\n", key);
		}
	}
	// printf("res: '%s'\n", res);
	return res;
}
/*
 * skiplist_map_get_prev_OID -- searches for prev OID of the key
 */
PMEMoid*
skiplist_map_get_prev_OID(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	char *key)
{	
	PMEMoid* res;
	int exactly_found_level = -1;
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];

	skiplist_map_get_prev_find(pop, key, map, path, &exactly_found_level);
	if (exactly_found_level != -1) {
		res = &(path[exactly_found_level].oid);
	} else {
		// FIXME: check prev to -1
		if (TOID_EQUALS(map, path[0])) {
			res = const_cast<PMEMoid *>(&OID_NULL);
		}
		else {
			res = &(path[0].oid);
		}
	}
	return res;
}
/*
 * skiplist_map_get_next_OID -- searches for next OID of the key
 */
PMEMoid*
skiplist_map_get_next_OID(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	char *key)
{	
	// PMEMoid *res = OID_NULL;
	PMEMoid *res;
	int exactly_found_level = -1;
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];

	skiplist_map_get_find(pop, key, map, path, &exactly_found_level);
	if (exactly_found_level != -1) {
		res = &(D_RW(path[exactly_found_level])->next[exactly_found_level].oid);
	} else {
		if (!TOID_EQUALS(path[0], NULL_NODE)) {
			res = &(D_RW(path[0])->next[0].oid);
		} else {
			res = const_cast<PMEMoid *>(&OID_NULL);
		}
	}
	return res;
}
/*
 * skiplist_map_get_first_OID -- searches for OID of first node
 */
PMEMoid*
skiplist_map_get_first_OID(PMEMobjpool *pop, TOID(struct skiplist_map_node) map)
{	
	// PMEMoid res = OID_NULL;
	PMEMoid *res;
	// TOID(struct skiplist_map_node) first = D_RO(map)->next[0];
	
	// Check whether first-node is valid
	// if (!TOID_EQUALS(first, NULL_NODE)) {
	uint8_t key_len = D_RO(D_RO(map)->next[0])->entry.key_len;
	uint8_t value_len = D_RO(D_RO(map)->next[0])->entry.value_len;
	if (key_len && value_len) {
		res = &(D_RW(map)->next[0].oid);
	}
	// }
	return res;
}
/*
 * skiplist_map_get_last_OID -- searches for OID of last node
 */
PMEMoid*
skiplist_map_get_last_OID(PMEMobjpool *pop, TOID(struct skiplist_map_node) map)
{	
	// PMEMoid res = OID_NULL;
	PMEMoid *res;

	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];
	// Seek non-empty node
	skiplist_map_get_last_find(pop, map, path);
	// found = path[0];
	// if (!TOID_EQUALS(found, NULL_NODE)) {
		// res = found.oid;
	res = &(path[0].oid);
	// } else {
	// 	printf("[Get_last_OID-ERROR]\n");
	// }
	return res;
}


/*
 * skiplist_map_lookup -- searches if a key exists
 */
int
skiplist_map_lookup(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	char *key)
{
	int ret = 0;
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM], found;
	// printf("00\n");
	skiplist_map_find(pop, key, map, path);
	
	// printf("11\n");
	found = D_RO(path[0])->next[0];
	// printf("22\n");
	if (!TOID_EQUALS(found, NULL_NODE)) {
	// printf("33\n");
		uint8_t key_len = D_RO(found)->entry.key_len+STRING_PADDING;
		char *buf = (char *)malloc(key_len);
	// printf("44\n");
		void *ptr = pmemobj_direct(D_RO(found)->entry.key);
		pmemobj_memcpy_persist(pop, buf, ptr, key_len);
	// printf("55\n");
		if (strcmp(buf, key) == 0)
			ret = 1;
	// printf("66\n");
		free(buf);
	}
	return ret;
}

/*
 * skiplist_map_foreach -- calls function for each node on a list
 */
int
skiplist_map_foreach(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	int (*cb)(char *key, char *value, int key_len, int value_len, void *arg), void *arg)
{
	TOID(struct skiplist_map_node) next = map;
	while (!TOID_EQUALS(D_RO(next)->next[0], NULL_NODE)) {
		next = D_RO(next)->next[0];
		void *key_ptr;
		void *value_ptr;
		if (D_RO(next)->entry.key_ptr != nullptr &&
				D_RO(next)->entry.value_ptr != nullptr) {
      key_ptr = D_RO(next)->entry.key_ptr;
      value_ptr = D_RO(next)->entry.value_ptr;
		} else {
			key_ptr = pmemobj_direct(D_RO(next)->entry.key);
			value_ptr = pmemobj_direct(D_RO(next)->entry.value);
		}

		char *res_key = new char[D_RO(next)->entry.key_len];
		char *res_value = new char[D_RO(next)->entry.value_len];

		// FIXME: Use proper function..
		strcpy(res_key, (char *)key_ptr);
		strcpy(res_value, (char *)value_ptr);
		
		// pmemobj_memcpy_persist(pop, res_key, key_ptr, 
		// 												D_RO(next)->entry.key_len);
		// pmemobj_memcpy_persist(pop, res_value, value_ptr, 
		// 												D_RO(next)->entry.value_len);

		// char *res_key = new char;
		// char *res_value = new char;
		// pmemobj_memcpy_persist(pop, res_key, key_ptr, 
		// 												D_RO(next)->entry.key_len);
		// pmemobj_memcpy_persist(pop, res_value, value_ptr, 
		// 												D_RO(next)->entry.value_len);

		cb(res_key, res_value, D_RO(next)->entry.key_len, D_RO(next)->entry.value_len, arg);
	}
	return 0;
}

/*
 * skiplist_map_is_empty -- checks whether the list map is empty
 */
int
skiplist_map_is_empty(PMEMobjpool *pop, TOID(struct skiplist_map_node) map)
{
	return TOID_IS_NULL(D_RO(map)->next[0]);
}

/*
 * skiplist_map_check -- check if given persistent object is a skiplist
 */
int
skiplist_map_check(PMEMobjpool *pop, TOID(struct skiplist_map_node) map)
{
	return TOID_IS_NULL(map) || !TOID_VALID(map);
}

} // namespace leveldb