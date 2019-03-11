// /*
//  * Copyright 2015-2018, Intel Corporation
//  *
//  * Redistribution and use in source and binary forms, with or without
//  * modification, are permitted provided that the following conditions
//  * are met:
//  *
//  *     * Redistributions of source code must retain the above copyright
//  *       notice, this list of conditions and the following disclaimer.
//  *
//  *     * Redistributions in binary form must reproduce the above copyright
//  *       notice, this list of conditions and the following disclaimer in
//  *       the documentation and/or other materials provided with the
//  *       distribution.
//  *
//  *     * Neither the name of the copyright holder nor the names of its
//  *       contributors may be used to endorse or promote products derived
//  *       from this software without specific prior written permission.
//  *
//  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  */

// /*
//  * map.h -- common interface for maps
//  */

// #ifndef MAP_H
// #define MAP_H

// #include <libpmemobj.h>
// #include <string>
// // #ifdef __cplusplus
// // extern "C" {
// // #endif

// #ifndef MAP_TYPE_OFFSET
// #define MAP_TYPE_OFFSET 1000
// #endif

// namespace leveldb {
// TOID_DECLARE(struct map, MAP_TYPE_OFFSET + 0);

// struct map;
// struct map_ctx;

// struct map_ops {
// 	int(*check)(PMEMobjpool *pop, TOID(struct map) map);
// 	int(*create)(PMEMobjpool *pop, TOID(struct map) *map, int index, void *arg);
// 	int(*destroy)(PMEMobjpool *pop, TOID(struct map) *map);
// 	int(*init)(PMEMobjpool *pop, TOID(struct map) map);
// 	int(*insert)(PMEMobjpool *pop, TOID(struct map) map,
// 		char *key, char *value, int key_len, int value_len, int index);
// 	int(*insert_by_oid)(PMEMobjpool *pop, TOID(struct map) map,
// 		PMEMoid *key_oid, PMEMoid *value_oid, int key_len, int value_len, int index);
// 	int(*insert_null_node)(PMEMobjpool *pop, TOID(struct map) map, int index);
// 	/* Deprecated function */
// 	int(*insert_new)(PMEMobjpool *pop, TOID(struct map) map,
// 		char *key, size_t size,
// 		unsigned type_num,
// 		void(*constructor)(PMEMobjpool *pop, void *ptr, void *arg),
// 		void *arg);
// 	char*(*remove)(PMEMobjpool *pop, TOID(struct map) map,
// 		char *key);
// 	int(*remove_free)(PMEMobjpool *pop, TOID(struct map) map,
// 		char *key);
// 	int(*clear)(PMEMobjpool *pop, TOID(struct map) map);
// 	char*(*get)(PMEMobjpool *pop, TOID(struct map) map, char *key);
// 	int(*lookup)(PMEMobjpool *pop, TOID(struct map) map,
// 		char *key);
// 	int(*foreach)(PMEMobjpool *pop, TOID(struct map) map,
// 		int(*cb)(char *key, char *value, void *arg),
// 		void *arg);
// 	int(*is_empty)(PMEMobjpool *pop, TOID(struct map) map);
// 	size_t(*count)(PMEMobjpool *pop, TOID(struct map) map);
// 	int(*cmd)(PMEMobjpool *pop, TOID(struct map) map,
// 		unsigned cmd, uint64_t arg);
// 	const PMEMoid*(*get_prev_OID)(PMEMobjpool *pop, TOID(struct map) map, char *key);
// 	const PMEMoid*(*get_next_OID)(PMEMobjpool *pop, TOID(struct map) map, char *key);
// 	const PMEMoid*(*get_first_OID)(PMEMobjpool *pop, TOID(struct map) map);
// 	const PMEMoid*(*get_last_OID)(PMEMobjpool *pop, TOID(struct map) map);
// };

// struct map_ctx {
// 	PMEMobjpool *pop;
// 	const struct map_ops *ops;
// };

// struct map_ctx *map_ctx_init(const struct map_ops *ops, PMEMobjpool *pop);
// void map_ctx_free(struct map_ctx *mapc);
// int map_check(struct map_ctx *mapc, TOID(struct map) map);
// int map_create(struct map_ctx *mapc, TOID(struct map) *map, int index, void *arg);
// int map_destroy(struct map_ctx *mapc, TOID(struct map) *map);
// int map_init(struct map_ctx *mapc, TOID(struct map) map);
// int map_insert(struct map_ctx *mapc, TOID(struct map) map,
// 	char *key, char *value, int key_len, int value_len, int index);
// int map_insert_by_oid(struct map_ctx *mapc, TOID(struct map) map,
// 	PMEMoid *key_oid, PMEMoid *value_oid, int key_len, int value_len, int index);
// int map_insert_null_node(struct map_ctx *mapc, TOID(struct map) map, int index);
// /* Deprecated function */
// int map_insert_new(struct map_ctx *mapc, TOID(struct map) map,
// 	char *key, size_t size,
// 	unsigned type_num,
// 	void(*constructor)(PMEMobjpool *pop, void *ptr, void *arg),
// 	void *arg);
// char* map_remove(struct map_ctx *mapc, TOID(struct map) map, char *key);
// int map_remove_free(struct map_ctx *mapc, TOID(struct map) map, char *key);
// int map_clear(struct map_ctx *mapc, TOID(struct map) map);
// char* map_get(struct map_ctx *mapc, TOID(struct map) map, char *key);
// int map_lookup(struct map_ctx *mapc, TOID(struct map) map, char *key);
// int map_foreach(struct map_ctx *mapc, TOID(struct map) map,
// 	int(*cb)(char *key, char *value, void *arg),
// 	void *arg);
// int map_is_empty(struct map_ctx *mapc, TOID(struct map) map);
// size_t map_count(struct map_ctx *mapc, TOID(struct map) map);
// int map_cmd(struct map_ctx *mapc, TOID(struct map) map,
// 	unsigned cmd, uint64_t arg);
// const PMEMoid*
// 	map_get_prev_OID(struct map_ctx *mapc, TOID(struct map) map, char *key);
// const PMEMoid* 
// 	map_get_next_OID(struct map_ctx *mapc, TOID(struct map) map, char *key);
// const PMEMoid*
// 	map_get_first_OID(struct map_ctx *mapc, TOID(struct map) map);
// const PMEMoid*
// 	map_get_last_OID(struct map_ctx *mapc, TOID(struct map) map);
// } // namespace leveldb
// // #ifdef __cplusplus
// // }
// // #endif

// #endif /* MAP_H */