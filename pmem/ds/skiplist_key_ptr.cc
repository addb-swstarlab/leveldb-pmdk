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
 * skiplist_map.c -- Skiplist implementation
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "pmem/ds/skiplist_key_ptr.h"

#include "util/coding.h" 

#include <chrono>
#include <iostream>

#define SKIPLIST_LEVELS_NUM 12
#define NULL_NODE TOID_NULL(struct skiplist_map_node)

namespace leveldb {

struct skiplist_map_entry {
	// PMEMoid key;
	// uint8_t key_len;
	// void* key_ptr;
	// TEST:
	char* buffer_ptr;
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
int common_constant;

//GetVarint32Ptr_delay

uint32_t GetKeyLengthFromBuffer(char* buf) {
	// Read encoded key-length
	uint32_t key_len;
	const char* key_ptr = GetVarint32Ptr(buf, buf+5, &key_len);
	// Get key
	return key_len;
}
char* GetKeyFromBuffer(char* buf) {
	// Read encoded key-length
	uint32_t key_len;
	const char* key_ptr = GetVarint32Ptr(buf, buf+5, &key_len);
	// Get key
	return const_cast<char *>(key_ptr);
}
char* GetKeyAndLengthFromBuffer(char* buf, uint32_t* key_len) {
	// Read encoded key-length
	const char* key_ptr = GetVarint32Ptr(buf, buf+5, key_len);
	// Get key
	return const_cast<char *>(key_ptr);
}
char* GetValueFromBuffer(char* buf) {
	// Read encoded key-length
	uint32_t key_length, value_length;
	// Skip key-part
	const char* key_ptr = GetVarint32Ptr(buf, buf+5, &key_length);
	// Read encoded value-length
	const char* value_ptr = GetVarint32Ptr(
																	buf+key_length+VarintLength(key_length),
																	buf+key_length+VarintLength(key_length)+5,
																	&value_length);
	// Get value
	return const_cast<char *>(value_ptr);                 
}
char* GetValueAndLengthFromBuffer(char* buf, uint32_t* value_len) {
	// Read encoded key-length
	uint32_t key_length, value_length;
	// Skip key-part
	const char* key_ptr = GetVarint32Ptr(buf, buf+5, &key_length);
	// Read encoded value-length
	const char* value_ptr = GetVarint32Ptr(
																	buf+key_length+VarintLength(key_length),
																	buf+key_length+VarintLength(key_length)+5,
																	value_len);
	// Get value
	return const_cast<char *>(value_ptr);                 
}
/*
 * skiplist_map_clear -- removes all elements from the map
 */
int
skiplist_map_clear(PMEMobjpool* pop, TOID(struct skiplist_map_node) map)
{
	TOID(struct skiplist_map_node) next = D_RO(map)->next[0];
	while (!TOID_EQUALS(next, NULL_NODE)) {
		// D_RW(next)->entry.key_len = 0;
		// D_RW(next)->entry.key_ptr = nullptr;
		D_RW(next)->entry.buffer_ptr = nullptr;
		next = D_RO(next)->next[0];
	}
	return 0;
}

/*
 * skiplist_map_destroy -- cleanups and frees skiplist instance
 */
int
skiplist_map_destroy(PMEMobjpool* pop, TOID(struct skiplist_map_node)* map)
{
	int ret = 0;
	TX_BEGIN(pop) {
		skiplist_map_clear(pop, *map);
		/* 
		 * TODO: [190222][JH], Need to fix this function call...
		 * Since this occurs error, it goes to TX_ONABORT state
		 */
		// pmemobj_tx_add_range_direct(map, sizeof(*map));
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
skiplist_map_find(PMEMobjpool* pop, char* key, 
	TOID(struct skiplist_map_node) map, TOID(struct skiplist_map_node)* path)
{
	int current_level;
	TOID(struct skiplist_map_node) active = map;
	for (current_level = SKIPLIST_LEVELS_NUM - 1;
			current_level >= 0; current_level--) {
		TOID(struct skiplist_map_node) next = D_RO(active)->next[current_level];
		for ( char *ptr ;
				!TOID_EQUALS(next, NULL_NODE);
				next = D_RO(active)->next[current_level]) {
			// uint8_t key_len = D_RO(next)->entry.key_len+STRING_PADDING;

			// ptr = pmemobj_direct(D_RO(next)->entry.key);
			// PROGRESS:
			ptr = GetKeyFromBuffer(D_RO(next)->entry.buffer_ptr);

			// printf("[DEBUG %d] key:'%s' ptr:'%s'\n", current_level, key, (char *)ptr);
			if (strcmp(key, ptr) <= 0)  { // ascending order
			// if (strcmp(key, (char *)ptr) >= 0)  { // descending order
			// printf("[Break DEBUG %d] key:'%s' ptr:'%s'\n", current_level, key, (char *)ptr);
				break;
			}
			active = next;
		}
		path[current_level] = active;
	}
}
/*
 * skiplist_map_insert_find -- (internal) returns path to last node, or if
 * node doesn't exist, it will return path to place where key should be.
 */
static void
skiplist_map_insert_find(PMEMobjpool* pop,
	TOID(struct skiplist_map_node) map, TOID(struct skiplist_map_node)* path)
{
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
}
/*
 * skiplist_map_insert -- inserts a new key-value pair into the map
 */
int
skiplist_map_insert(PMEMobjpool* pop, 
	TOID(struct skiplist_map_node) map, 
	TOID(struct skiplist_map_node)* current_node,
	char* key, char* buffer_ptr, int key_len, int index)
{
	// std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	
	int ret = 0;
	TOID(struct skiplist_map_node) new_node = D_RW(*current_node)->next[0];
	if (TOID_IS_NULL(new_node) || TOID_EQUALS(new_node, NULL_NODE)) {
		printf("[ERROR][Skiplist][insertion] Out of bound \n");
		ret = 1;
	}
	*current_node = new_node;
	// void *key_ptr = pmemobj_direct(D_RW(new_node)->entry.key);
	// pmemobj_memcpy_persist(pop, (char *)key_ptr, (char *)key, key_len);
	// D_RW(new_node)->entry.key_len = (uint8_t)key_len; 
	// PROGRESS: set buffer_ptr
	D_RW(new_node)->entry.buffer_ptr = buffer_ptr;
	// std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();
	// std::cout << "Insert = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";

	return ret;
}
/*
 *  skiplist_map_insert_by_oid -- inserts a specifics node into the map by kv oid
 */
int
skiplist_map_insert_by_oid(PMEMobjpool* pop, TOID(struct skiplist_map_node) map,
	TOID(struct skiplist_map_node)* current_node, 
	PMEMoid* key_oid, char* buffer_ptr, int key_len, int index)
{
	int ret = 0;
	TOID(struct skiplist_map_node) new_node = D_RW(*current_node)->next[0];
	if (TOID_IS_NULL(new_node) || TOID_EQUALS(new_node, NULL_NODE)) {
		printf("[ERROR][Skiplist][insertionByOID] Out of bound \n");
		ret = 1;
	}
	// D_RW(new_node)->entry.key = *key_oid;
	// D_RW(new_node)->entry.key_len = (uint8_t)key_len;
	
	// PROGRESS: set buffer_ptr
	D_RW(new_node)->entry.buffer_ptr = buffer_ptr;

	*current_node = new_node;
	return ret;
}
/*
 *  skiplist_map_insert_by_ptr -- inserts a specifics node into the map by kv ptr
 */
int
skiplist_map_insert_by_ptr(PMEMobjpool* pop, TOID(struct skiplist_map_node) map,
	TOID(struct skiplist_map_node) *current_node, 
	void* key_ptr, char* buffer_ptr, int key_len, int index)
{
	int ret = 0;
	TOID(struct skiplist_map_node) new_node = D_RW(*current_node)->next[0];
	if (TOID_IS_NULL(new_node) || TOID_EQUALS(new_node, NULL_NODE)) {
		printf("[ERROR][Skiplist][insertionByPTR] Out of bound \n");
		ret = 1;
	}
	// D_RW(new_node)->entry.key_ptr = key_ptr;
	// D_RW(new_node)->entry.key_len = (uint8_t)key_len;
	// PROGRESS: set buffer_ptr
	// TODO: Delete this function
	D_RW(new_node)->entry.buffer_ptr = buffer_ptr;

	*current_node = new_node;
	return ret;
}

/*
 *  skiplist_map_insert_null_node -- inserts null node at last 
 */
int
skiplist_map_insert_null_node(PMEMobjpool* pop, 
	TOID(struct skiplist_map_node) map,
	TOID(struct skiplist_map_node)* current_node, int index)
{
	int ret = 0;
	TOID(struct skiplist_map_node) null_node = NULL_NODE;
	D_RW(*current_node)->next[0] = null_node;
	*current_node = NULL_NODE;
	return ret;
}

/*
 * skiplist_map_create_insert -- (internal) inserts an empty key-value pair 
 * into the map. Just pre-allocate new-node and key&value area
 */
int
skiplist_map_create_insert(PMEMobjpool* pop, TOID(struct skiplist_map_node) map)
{
	int ret = 0;
	TOID(struct skiplist_map_node) new_node;
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];

	TX_BEGIN(pop) {
		new_node = TX_ZNEW(struct skiplist_map_node);
		// D_RW(new_node)->entry.key = pmemobj_tx_zalloc(PRE_ALLOC_KEY_SIZE, 500); // temp, string:= 500
		// D_RW(new_node)->entry.key_ptr = nullptr;
		D_RW(new_node)->entry.buffer_ptr = nullptr;

		skiplist_map_insert_find(pop, map, path);
		skiplist_map_insert_node(new_node, path);

	} TX_ONABORT {
		ret = 1;
	} TX_END
	return ret;
}
/*
 * skiplist_map_create -- allocates a new skiplist instance
 */
int
skiplist_map_create(PMEMobjpool* pop, TOID(struct skiplist_map_node)* map,
	TOID(struct skiplist_map_node) current_node,
	int index, void* arg)
{
	int ret = 0;
	// NOTE: initialize common_constant
	common_constant = 0;
	TX_BEGIN(pop) {
		/* Pre-allcate estimated-nodelength */
		if ((char *)arg != nullptr) {
			pmemobj_tx_add_range_direct(map, sizeof(*map));
			*map = TX_ZNEW(struct skiplist_map_node);
			for (int i=0; i< NUM_OF_PRE_ALLOC_NODE; i++) {
				// printf("i: %d\n", i);
				skiplist_map_create_insert(pop, *map);
			}
		}
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
skiplist_map_remove_free(PMEMobjpool* pop, TOID(struct skiplist_map_node) map,
	char* key)
{
	int ret = 0;
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];
	TOID(struct skiplist_map_node) to_remove;
	TX_BEGIN(pop) {
		skiplist_map_find(pop, key, map, path);
		to_remove = D_RO(path[0])->next[0];
		if (!TOID_EQUALS(to_remove, NULL_NODE)) {
			uint32_t key_len; //= D_RO(to_remove)->entry.key_len+STRING_PADDING;
			char *ptr = GetKeyAndLengthFromBuffer(D_RO(to_remove)->entry.buffer_ptr, &key_len);
			// char *buf = (char *)malloc(key_len);
			// void *ptr = pmemobj_direct(D_RO(to_remove)->entry.key);
			// pmemobj_memcpy_persist(pop, buf, ptr, key_len);
	// printf("[REMOVE_FREE-DEBUG] key:'%s' ptr:'%s' buf:'%s'\n", key, (char *)ptr, buf);
			if (memcmp(ptr, key, key_len) == 0) {
				// pmemobj_tx_free(D_RW(to_remove)->entry.key);
				skiplist_map_remove_node(path);
				ret = 1;
			} 
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
skiplist_map_remove(PMEMobjpool* pop, TOID(struct skiplist_map_node) map,
	char* key)
{
	char* ret = new char;
	strcpy(ret, "");
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];
	TOID(struct skiplist_map_node) to_remove;
	TX_BEGIN(pop) {
		skiplist_map_find(pop, key, map, path);
		to_remove = D_RO(path[0])->next[0];
		if (!TOID_EQUALS(to_remove, NULL_NODE)) {
			uint32_t key_len; //= D_RO(to_remove)->entry.key_len+STRING_PADDING;
			char *ptr = GetKeyAndLengthFromBuffer(D_RO(to_remove)->entry.buffer_ptr, &key_len);
			// char *buf = (char *)malloc(key_len);
			// void *ptr = pmemobj_direct(D_RO(to_remove)->entry.key);
			// pmemobj_memcpy_persist(pop, buf, ptr, key_len);
	// printf("[REMOVE-DEBUG] key:'%s' ptr:'%s' buf:'%s'\n", key, (char *)ptr, buf);
			if (memcmp(ptr, key, key_len) == 0) {
				// free(buf);
				skiplist_map_remove_node(path);
			} 
			// free(buf);
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
skiplist_map_get_find(PMEMobjpool* pop, char* key, 
	TOID(struct skiplist_map_node) map, TOID(struct skiplist_map_node)* path,
	int* find_res)
{
	int current_level;
	TOID(struct skiplist_map_node) active = map;
	for (current_level = SKIPLIST_LEVELS_NUM - 1;
			current_level >= 0; current_level--) {

		// void *key_ptr = pmemobj_direct(D_RO(active)->entry.key);
		// printf("[Before DEBUG %d] key:'%s' ptr:'%s' \n", current_level, key, (char *)key_ptr);

		TOID(struct skiplist_map_node) next = D_RO(active)->next[current_level];
		for ( char *ptr ;
				!TOID_EQUALS(next, NULL_NODE);
				next = D_RO(active)->next[current_level]) {
			if (D_RO(next)->entry.buffer_ptr == nullptr)
				break;
			uint32_t key_len; //= D_RO(next)->entry.key_len;
			ptr = GetKeyAndLengthFromBuffer(D_RO(next)->entry.buffer_ptr, &key_len);
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
			int res_cmp;
			// if (D_RO(next)->entry.key_ptr != nullptr) {
			// 	ptr = D_RO(next)->entry.key_ptr;
			// } else {
			// 	ptr = pmemobj_direct(D_RO(next)->entry.key);
			// }
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
}
/*
 * skiplist_map_get_prev_find -- (internal) returns path to searched node, or if
 * node doesn't exist, it will return path to place where key should be.
 */
static void
skiplist_map_get_prev_find(PMEMobjpool* pop, char* key, 
	TOID(struct skiplist_map_node) map, TOID(struct skiplist_map_node)* path,
	int* find_res)
{
	int current_level;
	TOID(struct skiplist_map_node) active = map;
	for (current_level = SKIPLIST_LEVELS_NUM - 1;
			current_level >= 0; current_level--) {

		// void *key_ptr = pmemobj_direct(D_RO(active)->entry.key);
		// printf("[Before DEBUG %d] key:'%s' ptr:'%s'\n", current_level, key, (char *)key_ptr);

		TOID(struct skiplist_map_node) next = D_RO(active)->next[current_level];
		for ( char *ptr ;
				!TOID_EQUALS(next, NULL_NODE);
				next = D_RO(active)->next[current_level]) {
			// NOTE: Key-matching exactly
			uint32_t key_len; // = D_RO(next)->entry.key_len;
			ptr = GetKeyAndLengthFromBuffer(D_RO(next)->entry.buffer_ptr, &key_len);
			// Avoid looping about empty&pre-allocated key
			if (key_len == 0) {
				break;
			}
			// ptr = pmemobj_direct(D_RO(next)->entry.key);
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
skiplist_map_get_last_find(PMEMobjpool* pop, 
	TOID(struct skiplist_map_node) map, TOID(struct skiplist_map_node)* path)
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
			uint32_t key_len = GetKeyLengthFromBuffer(D_RO(next)->entry.buffer_ptr);
			// Seek first empty node in each level
			if (key_len == 0) break;
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
skiplist_map_get(PMEMobjpool* pop, TOID(struct skiplist_map_node) map,
	char* key)
{	
	int exactly_found_level = -1;
	char* res;
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM], found;
	skiplist_map_get_find(pop, key, map, path, &exactly_found_level);
	if (exactly_found_level != -1) {
		found = D_RO(path[exactly_found_level])->next[exactly_found_level];
		// PROGRESS: set buffer_ptr
		res = D_RO(found)->entry.buffer_ptr;
		// printf("Get key %d: '%s'\n", exactly_found_level, key_ptr);
	} else {
		found = D_RO(path[0])->next[0];
		if (!TOID_EQUALS(found, NULL_NODE)) {
			// uint32_t key_len; //= D_RO(found)->entry.key_len+STRING_PADDING-NUM_OF_TAG_BYTES;
			// char *ptr = GetKeyAndLengthFromBuffer(D_RO(found)->entry.buffer_ptr, &key_len);
			// void *ptr = pmemobj_direct(D_RO(found)->entry.key);
		// printf("[GET-DEBUG] key:'%s' ptr:'%s' buf:'%s'\n", key, (char *)ptr, buf);
			// PROGRESS: set buffer_ptr
			res = D_RO(found)->entry.buffer_ptr;
				// printf("Get key %d : '%s'-'%s'\n", exactly_found_level, key, ptr);
		} else {
			res = nullptr;
			printf("[Get-ERROR] Reach first empty node '%s'\n", key);
		}
	}
	return res;
}
/*
 * skiplist_map_get_prev_OID -- searches for prev OID of the key
 */
PMEMoid*
skiplist_map_get_prev_OID(PMEMobjpool* pop, TOID(struct skiplist_map_node) map,
	char* key)
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
skiplist_map_get_next_OID(PMEMobjpool* pop, TOID(struct skiplist_map_node) map,
	char* key)
{	
	PMEMoid* res;
	int exactly_found_level = -1;
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];

	skiplist_map_get_find(pop, key, map, path, &exactly_found_level);
	if (exactly_found_level != -1) {
		res = const_cast<PMEMoid *>(&(D_RO(path[exactly_found_level])->next[exactly_found_level].oid));
	} else {
		if (!TOID_EQUALS(path[0], NULL_NODE)) {
			res = const_cast<PMEMoid *>(&(D_RO(path[0])->next[0].oid));
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
skiplist_map_get_first_OID(PMEMobjpool* pop, TOID(struct skiplist_map_node) map)
{	
	PMEMoid* res;
	// Check whether first-node is valid
	// uint8_t key_len = D_RO(D_RO(map)->next[0])->entry.key_len;
	uint32_t key_len = GetKeyLengthFromBuffer(D_RO(D_RO(map)->next[0])->entry.buffer_ptr);
	if (key_len) {
		res = &(D_RW(map)->next[0].oid);
	}
	return res;
}
/*
 * skiplist_map_get_last_OID -- searches for OID of last node
 */
PMEMoid*
skiplist_map_get_last_OID(PMEMobjpool* pop, TOID(struct skiplist_map_node) map)
{	
	PMEMoid* res;

	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];
	// Seek non-empty node
	skiplist_map_get_last_find(pop, map, path);
	res = &(path[0].oid);
	return res;
}


/*
 * skiplist_map_lookup -- searches if a key exists
 */
int
skiplist_map_lookup(PMEMobjpool* pop, TOID(struct skiplist_map_node) map,
	char* key)
{
	int ret = 0;
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM], found;
	skiplist_map_find(pop, key, map, path);
	
	found = D_RO(path[0])->next[0];
	if (!TOID_EQUALS(found, NULL_NODE)) {
		uint32_t key_len;// = D_RO(found)->entry.key_len+STRING_PADDING;
		char *ptr = GetKeyAndLengthFromBuffer(D_RO(found)->entry.buffer_ptr, &key_len);
		// char *buf = (char *)malloc(key_len);
		// void *ptr = pmemobj_direct(D_RO(found)->entry.key);
		// pmemobj_memcpy_persist(pop, buf, ptr, key_len);
		if (memcmp(ptr, key, key_len) == 0)
			ret = 1;
		// free(buf);
	}
	return ret;
}

/*
 * skiplist_map_foreach -- calls function for each node on a list
 */
int
skiplist_map_foreach(PMEMobjpool* pop, TOID(struct skiplist_map_node) map,
	int (*cb)(char* key, char* buffer_ptr, int key_len, void* arg), void* arg)
{
	TOID(struct skiplist_map_node) next = map;
	while (!TOID_EQUALS(D_RO(next)->next[0], NULL_NODE)) {
		next = D_RO(next)->next[0];
		// void* key_ptr;
		char* buffer_ptr = D_RO(next)->entry.buffer_ptr;
		uint32_t key_len;
		char *ptr = GetKeyAndLengthFromBuffer(D_RO(next)->entry.buffer_ptr, &key_len);

		// if (D_RO(next)->entry.key_ptr != nullptr) {
    //   key_ptr = D_RO(next)->entry.key_ptr;
    //   // PROGRESS: set buffer_ptr
		// 	// value_ptr = D_RO(next)->entry.value_ptr;
		// 	buffer_ptr = D_RO(next)->entry.buffer_ptr;
		// } else {
		// 	key_ptr = pmemobj_direct(D_RO(next)->entry.key);
    //   // PROGRESS: set buffer_ptr
		// 	buffer_ptr = D_RO(next)->entry.buffer_ptr;
		// }
		// char* res_key = new char[key_len];
		// FIXME: Use proper function..
		// strcpy(res_key, (char *)key_ptr);
		
		cb(ptr, buffer_ptr, key_len, arg);
	}
	return 0;
}

/*
 * skiplist_map_is_empty -- checks whether the list map is empty
 */
int
skiplist_map_is_empty(PMEMobjpool* pop, TOID(struct skiplist_map_node) map)
{
	return TOID_IS_NULL(D_RO(map)->next[0]);
}

/*
 * skiplist_map_check -- check if given persistent object is a skiplist
 */
int
skiplist_map_check(PMEMobjpool* pop, TOID(struct skiplist_map_node) map)
{
	return TOID_IS_NULL(map) || !TOID_VALID(map);
}

} // namespace leveldb