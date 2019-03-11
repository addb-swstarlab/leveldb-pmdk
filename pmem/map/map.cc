// /*
//  * Copyright 2015-2017, Intel Corporation
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
//  * map.c -- common interface for maps
//  */
// #include <stdlib.h>
// #include <stdio.h>
// #include <libpmemobj.h>

// #include "pmem/map/map.h"

// #define ABORT_NOT_IMPLEMENTED(mapc, func)\
// 	if ((mapc)->ops->func == NULL) {\
// 		fprintf(stderr, "error: '%s'"\
// 			" function not implemented\n", #func);\
// 		exit(1);\
// 	}

// namespace leveldb {
// /*
//  * map_ctx_init -- initialize map context
//  */
// struct map_ctx *
// map_ctx_init(const struct map_ops *ops, PMEMobjpool *pop)
// {
// 	if (!ops)
// 		return NULL;

// 	struct map_ctx *mapc = (struct map_ctx *)calloc(1, sizeof(*mapc));
// 	if (!mapc)
// 		return NULL;

// 	mapc->ops = ops;
// 	mapc->pop = pop;

// 	return mapc;
// }

// /*
//  * map_ctx_free -- free map context
//  */
// void
// map_ctx_free(struct map_ctx *mapc)
// {
// 	free(mapc);
// }

// /*
//  * map_create -- create new map
//  */
// int
// map_create(struct map_ctx *mapc, TOID(struct map) *map, int index, void *arg)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, create);
// 	return mapc->ops->create(mapc->pop, map, index, arg);
// }

// /*
//  * map_destroy -- free the map
//  */
// int
// map_destroy(struct map_ctx *mapc, TOID(struct map) *map)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, destroy);
// 	return mapc->ops->destroy(mapc->pop, map);
// }

// /*
//  * map_init -- initialize map
//  */
// int
// map_init(struct map_ctx *mapc, TOID(struct map) map)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, init);
// 	return mapc->ops->init(mapc->pop, map);
// }

// /*
//  * map_check -- check if persistent object is a valid map object
//  */
// int
// map_check(struct map_ctx *mapc, TOID(struct map) map)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, check);
// 	return mapc->ops->check(mapc->pop, map);
// }

// /*
//  * map_insert -- insert key value pair
//  */
// int
// map_insert(struct map_ctx *mapc, TOID(struct map) map,
// 	char *key, char *value, int key_len, int value_len, int index)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, insert);
// 	return mapc->ops->insert(mapc->pop, map, key, value, 
// 														key_len, value_len, index);
// }

// /*
//  * map_insert_by_oid -- insert by kv-oid
//  */
// int
// map_insert_by_oid(struct map_ctx *mapc, TOID(struct map) map,
// 	PMEMoid *key_oid, PMEMoid *value_oid, int key_len, int value_len, int index)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, insert_by_oid);
// 	return mapc->ops->insert_by_oid(mapc->pop, map, key_oid, value_oid, key_len,
// 		value_len, index);
// }

// /*
//  * map_insert_null_node -- insert null node at last
//  */
// int
// map_insert_null_node(struct map_ctx *mapc, TOID(struct map) map, int index)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, insert);
// 	return mapc->ops->insert_null_node(mapc->pop, map, index);
// }

// /*
//  * map_insert_new -- allocate and insert key value pair
//  */
// int
// map_insert_new(struct map_ctx *mapc, TOID(struct map) map,
// 		char *key, size_t size, unsigned type_num,
// 		void (*constructor)(PMEMobjpool *pop, void *ptr, void *arg),
// 		void *arg)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, insert_new);
// 	return mapc->ops->insert_new(mapc->pop, map, key, size,
// 			type_num, constructor, arg);
// }

// /*
//  * map_remove -- remove key value pair
//  */
// char*
// map_remove(struct map_ctx *mapc, TOID(struct map) map, char *key)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, remove);
// 	return mapc->ops->remove(mapc->pop, map, key);
// }

// /*
//  * map_remove_free -- remove and free key value pair
//  */
// int
// map_remove_free(struct map_ctx *mapc, TOID(struct map) map, char *key)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, remove_free);
// 	return mapc->ops->remove_free(mapc->pop, map, key);
// }

// /*
//  * map_clear -- remove all key value pairs from map
//  */
// int
// map_clear(struct map_ctx *mapc, TOID(struct map) map)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, clear);
// 	return mapc->ops->clear(mapc->pop, map);
// }

// /*
//  * map_get -- get value of specified key
//  */
// char*
// map_get(struct map_ctx *mapc, TOID(struct map) map, char *key)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, get);
// 	return mapc->ops->get(mapc->pop, map, key);
// }

// /*
//  * map_lookup -- check if specified key exists in map
//  */
// int
// map_lookup(struct map_ctx *mapc, TOID(struct map) map, char *key)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, lookup);
// 	return mapc->ops->lookup(mapc->pop, map, key);
// }

// /*
//  * map_foreach -- iterate through all key value pairs in a map
//  */
// int
// map_foreach(struct map_ctx *mapc, TOID(struct map) map,
// 		int (*cb)(char *key, char *value, void *arg),
// 		void *arg)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, foreach);
// 	return mapc->ops->foreach(mapc->pop, map, cb, arg);
// }

// /*
//  * map_is_empty -- check if map is empty
//  */
// int
// map_is_empty(struct map_ctx *mapc, TOID(struct map) map)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, is_empty);
// 	return mapc->ops->is_empty(mapc->pop, map);
// }

// /*
//  * map_count -- get number of key value pairs in map
//  */
// size_t
// map_count(struct map_ctx *mapc, TOID(struct map) map)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, count);
// 	return mapc->ops->count(mapc->pop, map);
// }

// /*
//  * map_cmd -- execute command specific for map type
//  */
// int
// map_cmd(struct map_ctx *mapc, TOID(struct map) map, unsigned cmd, uint64_t arg)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, cmd);
// 	return mapc->ops->cmd(mapc->pop, map, cmd, arg);
// }
// /*
//  * map_get_prev_OID -- get OID of specified key
//  */
// const PMEMoid*
// map_get_prev_OID(struct map_ctx *mapc, TOID(struct map) map, char *key)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, get_prev_OID);
// 	return mapc->ops->get_prev_OID(mapc->pop, map, key);
// }
// /*
//  * map_get_next_OID -- get OID of specified key
//  */
// const PMEMoid*
// map_get_next_OID(struct map_ctx *mapc, TOID(struct map) map, char *key)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, get_next_OID);
// 	return mapc->ops->get_next_OID(mapc->pop, map, key);
// }
// /*
//  * map_get_first_OID -- get OID of first node
//  */
// const PMEMoid*
// map_get_first_OID(struct map_ctx *mapc, TOID(struct map) map)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, get_first_OID);
// 	return mapc->ops->get_first_OID(mapc->pop, map);
// }
// /*
//  * map_get_first_OID -- get OID of last node
//  */
// const PMEMoid*
// map_get_last_OID(struct map_ctx *mapc, TOID(struct map) map)
// {
// 	ABORT_NOT_IMPLEMENTED(mapc, get_last_OID);
// 	return mapc->ops->get_last_OID(mapc->pop, map);
// }

// } // namespace leveldb