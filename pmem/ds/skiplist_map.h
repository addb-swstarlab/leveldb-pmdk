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
 * skiplist_map.h -- sorted list collection implementation
 */

#ifndef SKIPLIST_MAP_H
#define SKIPLIST_MAP_H

#include <libpmemobj.h>
#include <string>

#ifndef SKIPLIST_MAP_TYPE_OFFSET
#define SKIPLIST_MAP_TYPE_OFFSET 2020
#endif

#define NUM_OF_PRE_ALLOC_NODE 28300 // TODO: Need to adjust
#define PRE_ALLOC_KEY_SIZE 24 // 16
#define PRE_ALLOC_VALUE_SIZE 100 // 120
#define STRING_PADDING 0 // \0
// #define INSERT_PADDING 5 // kHeader, etc.
#define NUM_OF_TAG_BYTES 8

// #ifndef STRING_TYPE_OFFSET
// #define STRING_TYPE_OFFSET 2020
// #endif

namespace leveldb{

struct skiplist_map_node;
TOID_DECLARE(struct skiplist_map_node, SKIPLIST_MAP_TYPE_OFFSET + 0);

int skiplist_map_check(PMEMobjpool *pop, TOID(struct skiplist_map_node) map);
int skiplist_map_create(PMEMobjpool *pop, TOID(struct skiplist_map_node) *map,
	TOID(struct skiplist_map_node)* current_node, int index, void *arg);
int skiplist_map_destroy(PMEMobjpool *pop, TOID(struct skiplist_map_node) *map);
int skiplist_map_insert(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	TOID(struct skiplist_map_node)* current_node,	char *key, char *value, int key_len, int value_len, int index);
int skiplist_map_insert_by_oid(PMEMobjpool *pop, 
		TOID(struct skiplist_map_node) map, TOID(struct skiplist_map_node)* current_node,
		PMEMoid* node, PMEMoid *key_oid, PMEMoid *value_oid, int key_len, int value_len,
		int index);
int skiplist_map_insert_null_node(PMEMobjpool *pop, 
		TOID(struct skiplist_map_node) map, int index);
/* Deprecated function */
// int skiplist_map_insert_new(PMEMobjpool *pop,
// 		TOID(struct skiplist_map_node) map, char *key, size_t size,
// 		unsigned type_num,
// 		void (*constructor)(PMEMobjpool *pop, void *ptr, void *arg),
// 		void *arg);
char* skiplist_map_remove(PMEMobjpool *pop,
		TOID(struct skiplist_map_node) map, char *key);
int skiplist_map_remove_free(PMEMobjpool *pop,
		TOID(struct skiplist_map_node) map, char *key);
int skiplist_map_clear(PMEMobjpool *pop, TOID(struct skiplist_map_node) map);
char* skiplist_map_get(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
		char *key);
int skiplist_map_lookup(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
		char *key);
int skiplist_map_foreach(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	int (*cb)(char *key, char *value, void *arg), void *arg);
int skiplist_map_is_empty(PMEMobjpool *pop, TOID(struct skiplist_map_node) map);

// JH
const PMEMoid*
skiplist_map_get_prev_OID(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
		char *key);
const PMEMoid*
skiplist_map_get_next_OID(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
		char *key);
const PMEMoid*
skiplist_map_get_first_OID(PMEMobjpool *pop, TOID(struct skiplist_map_node) map);
const PMEMoid*
skiplist_map_get_last_OID(PMEMobjpool *pop, TOID(struct skiplist_map_node) map);

void
skiplist_map_get_next_TOID(PMEMobjpool *pop, 
		TOID(struct skiplist_map_node) map,
		char *key, 
		TOID(struct skiplist_map_node) *prev, 
		TOID(struct skiplist_map_node) *curr);

} // namespace leveldb

#endif /* SKIPLIST_MAP_H */