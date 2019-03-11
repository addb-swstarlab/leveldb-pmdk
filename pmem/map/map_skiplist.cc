// /*
//  * Copyright 2016, Intel Corporation
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
//  * map_skiplist.c -- common interface for maps
//  */

// #include "pmem/map/map.h"
// #include "pmem/ds/skiplist_map.h"

// #include "pmem/map/map_skiplist.h"

// namespace leveldb {
// /*
//  * map_skiplist_check -- wrapper for skiplist_map_check
//  */
// static int
// map_skiplist_check(PMEMobjpool *pop, TOID(struct map) map)
// {
// 	TOID(struct skiplist_map_node) skiplist_map;
// 	TOID_ASSIGN(skiplist_map, map.oid);

// 	return skiplist_map_check(pop, skiplist_map);
// }

// /*
//  * map_skiplist_create -- wrapper for skiplist_map_new
//  */
// static int
// map_skiplist_create(PMEMobjpool *pop, TOID(struct map) *map, int index, void *arg)
// {
// 	TOID(struct skiplist_map_node) *skiplist_map =
// 		(TOID(struct skiplist_map_node) *)map;

// 	// FIXME:
// 	// return skiplist_map_create(pop, skiplist_map, index, arg);
// 	return skiplist_map_create(pop, skiplist_map, *skiplist_map, index, arg);
// }

// /*
//  * map_skiplist_destroy -- wrapper for skiplist_map_delete
//  */
// static int
// map_skiplist_destroy(PMEMobjpool *pop, TOID(struct map) *map)
// {
// 	TOID(struct skiplist_map_node) *skiplist_map =
// 		(TOID(struct skiplist_map_node) *)map;

// 	return skiplist_map_destroy(pop, skiplist_map);
// }

// /*
//  * map_skiplist_insert -- wrapper for skiplist_map_insert
//  */
// static int
// map_skiplist_insert(PMEMobjpool *pop, TOID(struct map) map,
// 		char *key, char *value, int key_len, int value_len, int index)
// {
// 	TOID(struct skiplist_map_node) skiplist_map;
// 	TOID_ASSIGN(skiplist_map, map.oid);

// 	// FIXME:
// 	// return skiplist_map_insert(pop, skiplist_map, key, value, 
// 	// 															key_len, value_len, index);
// 	return skiplist_map_insert(pop, skiplist_map, nullptr, key, value, 
// 																key_len, value_len, index);
// }

// /*
//  * map_skiplist_insert_by_oid -- wrapper for skiplist_map_insert_by_oid
//  */
// static int
// map_skiplist_insert_by_oid(PMEMobjpool *pop, TOID(struct map) map, 
// 	PMEMoid *key_oid, PMEMoid *value_oid, int key_len, int value_len, int index)
// {
// 	TOID(struct skiplist_map_node) skiplist_map;
// 	TOID_ASSIGN(skiplist_map, map.oid);

// 	// FIXME:
// 	// return skiplist_map_insert_by_oid(pop, skiplist_map, key_oid, value_oid, key_len,
// 	// 	value_len, index);
// 	return skiplist_map_insert_by_oid(pop, skiplist_map, nullptr, 
// 		key_oid, value_oid, key_len,
// 		value_len, index);
// }

// /*
//  * map_skiplist_insert_null_node -- wrapper for skiplist_map_insert_null_node
//  */
// static int
// map_skiplist_insert_null_node(PMEMobjpool *pop, TOID(struct map) map, int index)
// {
// 	TOID(struct skiplist_map_node) skiplist_map;
// 	TOID_ASSIGN(skiplist_map, map.oid);

// 	// FIXME:
// 	// return skiplist_map_insert_null_node(pop, skiplist_map, index);
// 	return skiplist_map_insert_null_node(pop, skiplist_map, nullptr, index);
// }

// /*
//  * map_skiplist_insert_new -- wrapper for skiplist_map_insert_new
//  */
// /* Deprecated function */
// // static int
// // map_skiplist_insert_new(PMEMobjpool *pop, TOID(struct map) map,
// // 		char *key, size_t size,
// // 		unsigned type_num,
// // 		void (*constructor)(PMEMobjpool *pop, void *ptr, void *arg),
// // 		void *arg)
// // {
// // 	TOID(struct skiplist_map_node) skiplist_map;
// // 	TOID_ASSIGN(skiplist_map, map.oid);

// // 	return skiplist_map_insert_new(pop, skiplist_map, key, size,
// // 			type_num, constructor, arg);
// // }

// /*
//  * map_skiplist_remove -- wrapper for skiplist_map_remove
//  */
// static char*
// map_skiplist_remove(PMEMobjpool *pop, TOID(struct map) map, char *key)
// {
// 	TOID(struct skiplist_map_node) skiplist_map;
// 	TOID_ASSIGN(skiplist_map, map.oid);

// 	return skiplist_map_remove(pop, skiplist_map, key);
// }

// /*
//  * map_skiplist_remove_free -- wrapper for skiplist_map_remove_free
//  */
// static int
// map_skiplist_remove_free(PMEMobjpool *pop, TOID(struct map) map, char *key)
// {
// 	TOID(struct skiplist_map_node) skiplist_map;
// 	TOID_ASSIGN(skiplist_map, map.oid);

// 	return skiplist_map_remove_free(pop, skiplist_map, key);
// }

// /*
//  * map_skiplist_clear -- wrapper for skiplist_map_clear
//  */
// static int
// map_skiplist_clear(PMEMobjpool *pop, TOID(struct map) map)
// {
// 	TOID(struct skiplist_map_node) skiplist_map;
// 	TOID_ASSIGN(skiplist_map, map.oid);

// 	return skiplist_map_clear(pop, skiplist_map);
// }

// /*
//  * map_skiplist_get -- wrapper for skiplist_map_get
//  */
// static char*
// map_skiplist_get(PMEMobjpool *pop, TOID(struct map) map, char *key)
// {
// 	TOID(struct skiplist_map_node) skiplist_map;
// 	TOID_ASSIGN(skiplist_map, map.oid);

// 	return skiplist_map_get(pop, skiplist_map, key);
// }

// /*
//  * map_skiplist_lookup -- wrapper for skiplist_map_lookup
//  */
// static int
// map_skiplist_lookup(PMEMobjpool *pop, TOID(struct map) map, char *key)
// {
// 	TOID(struct skiplist_map_node) skiplist_map;
// 	TOID_ASSIGN(skiplist_map, map.oid);

// 	return skiplist_map_lookup(pop, skiplist_map, key);
// }

// /*
//  * map_skiplist_foreach -- wrapper for skiplist_map_foreach
//  */
// static int
// map_skiplist_foreach(PMEMobjpool *pop, TOID(struct map) map,
// 		int (*cb)(char *key, char *value, void *arg),
// 		void *arg)
// {
// 	TOID(struct skiplist_map_node) skiplist_map;
// 	TOID_ASSIGN(skiplist_map, map.oid);

// 	return skiplist_map_foreach(pop, skiplist_map, cb, arg);
// }

// /*
//  * map_skiplist_is_empty -- wrapper for skiplist_map_is_empty
//  */
// static int
// map_skiplist_is_empty(PMEMobjpool *pop, TOID(struct map) map)
// {
// 	TOID(struct skiplist_map_node) skiplist_map;
// 	TOID_ASSIGN(skiplist_map, map.oid);
	
// 	return skiplist_map_is_empty(pop, skiplist_map);
// }
// /*
//  * map_skiplist_get_prev_OID -- wrapper for skiplist_map_get_prev_OID
//  */
// const PMEMoid*
// map_skiplist_get_prev_OID(PMEMobjpool *pop, TOID(struct map) map, char *key)
// {
// 	TOID(struct skiplist_map_node) skiplist_map;
// 	TOID_ASSIGN(skiplist_map, map.oid);
	
// 	return skiplist_map_get_prev_OID(pop, skiplist_map, key);
// }
// /*
//  * map_skiplist_get_next_OID -- wrapper for skiplist_map_get_next_OID
//  */
// const PMEMoid*
// map_skiplist_get_next_OID(PMEMobjpool *pop, TOID(struct map) map, char *key)
// {
// 	TOID(struct skiplist_map_node) skiplist_map;
// 	TOID_ASSIGN(skiplist_map, map.oid);
	
// 	return skiplist_map_get_next_OID(pop, skiplist_map, key);
// }
// /*
//  * map_skiplist_get_first_OID -- wrapper for skiplist_map_get_first_OID
//  */
// const PMEMoid*
// map_skiplist_get_first_OID(PMEMobjpool *pop, TOID(struct map) map)
// {
// 	TOID(struct skiplist_map_node) skiplist_map;
// 	TOID_ASSIGN(skiplist_map, map.oid);
	
// 	return skiplist_map_get_first_OID(pop, skiplist_map);
// }
// /*
//  * map_skiplist_get_last_OID -- wrapper for skiplist_map_get_last_OID
//  */
// const PMEMoid*
// map_skiplist_get_last_OID(PMEMobjpool *pop, TOID(struct map) map)
// {
// 	TOID(struct skiplist_map_node) skiplist_map;
// 	TOID_ASSIGN(skiplist_map, map.oid);
	
// 	return skiplist_map_get_last_OID(pop, skiplist_map);
// }
// struct map_ops skiplist_map_ops = {
// 	/* .check	= */ map_skiplist_check,
// 	/* .create	= */ map_skiplist_create,
// 	/* .destroy	= */ map_skiplist_destroy,
// 	/* .init	= */ NULL,
// 	/* .insert	= */ map_skiplist_insert,
// 	/* .insert_by_oid	= */ map_skiplist_insert_by_oid,
// 	/* .insert_null_node	= */ map_skiplist_insert_null_node,
// 	/* .insert_new	= */ NULL, /* Deprecated function (map_skiplist_insert_new) */ 
// 	/* .remove	= */ map_skiplist_remove,
// 	/* .remove_free	= */ map_skiplist_remove_free,
// 	/* .clear	= */ map_skiplist_clear,
// 	/* .get		= */ map_skiplist_get,
// 	/* .lookup	= */ map_skiplist_lookup,
// 	/* .foreach	= */ map_skiplist_foreach,
// 	/* .is_empty	= */ map_skiplist_is_empty,
// 	/* .count	= */ NULL,
// 	/* .cmd		= */ NULL,
// 	/* .get_prev_OID = */ map_skiplist_get_prev_OID,
// 	/* .get_next_OID = */ map_skiplist_get_next_OID,
// 	/* .get_first_OID = */ map_skiplist_get_first_OID,
// 	/* .get_last_OID = */ map_skiplist_get_last_OID,
// };

// } // namespace leveldb