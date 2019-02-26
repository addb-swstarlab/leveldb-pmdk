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
#include "pmem/ds/skiplist_map.h"

#include <chrono>
#include <iostream>

#define SKIPLIST_LEVELS_NUM 4
#define NULL_NODE TOID_NULL(struct skiplist_map_node)

namespace leveldb {

struct skiplist_map_entry {
	PMEMoid key;
	uint8_t key_len;
	PMEMoid value;
	uint8_t value_len; // need to find
};

struct skiplist_map_node {
	TOID(struct skiplist_map_node) next[SKIPLIST_LEVELS_NUM];
	struct skiplist_map_entry entry;
};

/* LAST_NODE for find insert-position(next) */
struct store_last_node {
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];
};
struct store_last_node last_node;

/*
 * skiplist_map_create -- allocates a new skiplist instance
 */
int
skiplist_map_create(PMEMobjpool *pop, TOID(struct skiplist_map_node) *map,
	void *arg)
{
	int ret = 0;

	TX_BEGIN(pop) {
		// printf("[DEBUG1]");
		pmemobj_tx_add_range_direct(map, sizeof(*map));
		// printf(" - [DEBUG2]");
		*map = TX_ZNEW(struct skiplist_map_node);
		// printf(" - [DEBUG3] \n");
	} TX_ONABORT {
		ret = 1;
	} TX_END
	return ret;
}

/*
 * skiplist_map_clear -- removes all elements from the map
 */
int
skiplist_map_clear(PMEMobjpool *pop, TOID(struct skiplist_map_node) map)
{
	while (!TOID_EQUALS(D_RO(map)->next[0], NULL_NODE)) {
		TOID(struct skiplist_map_node) next = D_RO(map)->next[0];

		uint8_t key_len = D_RO(next)->entry.key_len+1;
		char *buf = (char *)malloc(key_len);
		void *ptr = pmemobj_direct(D_RO(next)->entry.key);
		pmemobj_memcpy_persist(pop, buf, ptr, key_len);
		skiplist_map_remove_free(pop, map, buf);
		free(buf);
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
 * skiplist_map_insert_new -- allocates a new object and inserts it into
 * the list
 * [Deprecated function]
 */
// int
// skiplist_map_insert_new(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
// 	char *key, size_t size, unsigned type_num,
// 	void (*constructor)(PMEMobjpool *pop, void *ptr, void *arg),
// 	void *arg)
// {
// 	int ret = 0;

// 	TX_BEGIN(pop) {
// 		PMEMoid n = pmemobj_tx_alloc(size, type_num); // value
// 		constructor(pop, pmemobj_direct(n), arg);
// 		skiplist_map_insert(pop, map, key, n);
// 	} TX_ONABORT {
// 		ret = 1;
// 	} TX_END

// 	return ret;
// }

/*
 * skiplist_map_insert_node -- (internal) adds new node in selected place
 */
static void
skiplist_map_insert_node(TOID(struct skiplist_map_node) new_node,
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM])
{
	// std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	unsigned current_level = 0;
	do {
		TX_ADD_FIELD(path[current_level], next[current_level]);
		D_RW(new_node)->next[current_level] =
			D_RO(path[current_level])->next[current_level];
		D_RW(path[current_level])->next[current_level] = new_node;
	} while (++current_level < SKIPLIST_LEVELS_NUM && rand() % 2 == 0);
	
	// std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();
	// std::cout << "insert-node = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<std::endl;
}

/*
 * skiplist_map_map_find -- (internal) returns path to searched node, or if
 * node doesn't exist, it will return path to place where key should be.
 */
static void
skiplist_map_find(PMEMobjpool *pop, char *key, 
	TOID(struct skiplist_map_node) map, TOID(struct skiplist_map_node) *path)
{
	// std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	int current_level;
	TOID(struct skiplist_map_node) active = map;
	for (current_level = SKIPLIST_LEVELS_NUM - 1;
			current_level >= 0; current_level--) {
		TOID(struct skiplist_map_node) next = D_RO(active)->next[current_level];
		for ( void *ptr ;
				!TOID_EQUALS(next, NULL_NODE);
				next = D_RO(active)->next[current_level]) {
			uint8_t key_len = D_RO(next)->entry.key_len+1;
			ptr = pmemobj_direct(D_RO(next)->entry.key);
			if (strcmp(key, (char *)ptr) <= 0)  { // ascending order
			// if (strcmp(key, (char *)ptr) >= 0)  { // descending order
				break;
			}
			printf("[DEBUG %d] key:'%s' ptr:'%s'\n", current_level, key, (char *)ptr);
			active = next;
		}
		path[current_level] = active;
	}
	// std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();
	// std::cout << "find = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<" / ";
}
/*
 * skiplist_map_find_insert -- (internal) returns path to last node, or if
 * node doesn't exist, it will return path to place where key should be.
 */
static void
skiplist_map_find_insert(PMEMobjpool *pop, char *key, 
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
 */
int
skiplist_map_insert(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	char *key, char *value)
	// char *key, PMEMoid value)
{
	int ret = 0;
	TOID(struct skiplist_map_node) new_node;
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];

	std::chrono::steady_clock::time_point begin, end;
	TX_BEGIN(pop) {
		// begin = std::chrono::steady_clock::now();
		// printf("New");
		new_node = TX_ZNEW(struct skiplist_map_node);
	// 	end= std::chrono::steady_clock::now();
	// std::cout << "insert1 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<" / ";
	// 	begin = std::chrono::steady_clock::now();
		// printf("-key");
		D_RW(new_node)->entry.key = pmemobj_tx_zalloc(strlen(key), 500); // temp, string:= 500
	// 	end= std::chrono::steady_clock::now();
	// std::cout << "insert2 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<" / ";
	// 	begin = std::chrono::steady_clock::now();
		void *key_ptr = pmemobj_direct(D_RW(new_node)->entry.key);
	// 	end= std::chrono::steady_clock::now();
	// std::cout << "insert3 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<" / ";
	// 	begin = std::chrono::steady_clock::now();
		pmemobj_memcpy_persist(pop, (char *)key_ptr, (char *)key, strlen(key)+1);
	// 	end= std::chrono::steady_clock::now();
	// std::cout << "insert4 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<" / ";
	// 	begin = std::chrono::steady_clock::now();
		D_RW(new_node)->entry.key_len = (uint8_t)strlen(key)+1; 
	// 	end= std::chrono::steady_clock::now();
	// std::cout << "insert5 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";

		// printf("-value");
		// begin = std::chrono::steady_clock::now();
		D_RW(new_node)->entry.value = pmemobj_tx_alloc(strlen(value), 600); // temp, string:= 500
	// 	end= std::chrono::steady_clock::now();
	// std::cout << "insert1 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<" / ";
		// begin = std::chrono::steady_clock::now();
		void *value_ptr = pmemobj_direct(D_RW(new_node)->entry.value);
	// 	end= std::chrono::steady_clock::now();
	// std::cout << "insert2 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<" / ";
	// 	begin = std::chrono::steady_clock::now();
		pmemobj_memcpy_persist(pop, (char *)value_ptr, (char *)value, strlen(value)+1);
	// 	end= std::chrono::steady_clock::now();
	// std::cout << "insert3 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";
	// 	begin = std::chrono::steady_clock::now();
		D_RW(new_node)->entry.value_len = (uint8_t)strlen(value)+1; 
	// 	end= std::chrono::steady_clock::now();
	// std::cout << "insert2 = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<" / ";
		printf("keylen:%d, valuelen:%d\n",D_RW(new_node)->entry.key_len, D_RW(new_node)->entry.value_len);
 		// printf("-find");
		skiplist_map_find_insert(pop, key, map, path);
		// printf("-insertNode");
		skiplist_map_insert_node(new_node, path);
		// printf("-End\n");

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
			uint8_t key_len = D_RO(to_remove)->entry.key_len+1;
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
			uint8_t key_len = D_RO(to_remove)->entry.key_len+1;
			char *buf = (char *)malloc(key_len);
			void *ptr = pmemobj_direct(D_RO(to_remove)->entry.key);
			pmemobj_memcpy_persist(pop, buf, ptr, key_len);
	// printf("[REMOVE-DEBUG] key:'%s' ptr:'%s' buf:'%s'\n", key, (char *)ptr, buf);
			if (strcmp(buf, key) == 0) {
		// printf("66\n");
				free(buf);
				uint8_t value_len = D_RO(to_remove)->entry.value_len+1;
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
 * skiplist_map_get -- searches for a value of the key
 */
char*
skiplist_map_get(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	char *key)
{
	char *ret = new char;
	strcpy(ret, "");
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM], found;
	skiplist_map_find(pop, key, map, path);
	found = D_RO(path[0])->next[0];
	if (!TOID_EQUALS(found, NULL_NODE)) {
		uint8_t key_len = D_RO(found)->entry.key_len+1;
		char *buf = (char *)malloc(key_len);
		void *ptr = pmemobj_direct(D_RO(found)->entry.key);
		pmemobj_memcpy_persist(pop, buf, ptr, key_len);
	// printf("[GET-DEBUG] key:'%s' ptr:'%s' buf:'%s'\n", key, (char *)ptr, buf);
		if (strcmp(buf, key) == 0) {
	// printf("66\n");
			free(buf);
			uint8_t value_len = D_RO(found)->entry.value_len+1;
			buf = (char *)malloc(value_len);
			ptr = pmemobj_direct(D_RO(found)->entry.value);
			pmemobj_memcpy_persist(pop, buf, ptr, value_len);
			strcpy(ret, buf);
		} 
		free(buf);
		// ret = D_RO(found)->entry.value;
	}
	return ret;
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
		uint8_t key_len = D_RO(found)->entry.key_len+1;
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
	int (*cb)(char *key, char *value, void *arg), void *arg)
{
	TOID(struct skiplist_map_node) next = map;
	while (!TOID_EQUALS(D_RO(next)->next[0], NULL_NODE)) {
		next = D_RO(next)->next[0];
		void *key_ptr = pmemobj_direct(D_RO(next)->entry.key);
		void *value_ptr = pmemobj_direct(D_RO(next)->entry.value);

		char *res_key = new char[D_RO(next)->entry.key_len];
		char *res_value = new char[D_RO(next)->entry.value_len];

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

		cb(res_key, res_value, arg);
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