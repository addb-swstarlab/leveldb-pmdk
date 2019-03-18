/*
 * Copyright 2015-2018, Intel Corporation
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

/* integer hash set implementation which uses only atomic APIs */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#include <string>
#include <chrono>
#include <iostream>

#include <libpmemobj.h>
#include "pmem/ds/hashmap_atomic.h"
#include "pmem/ds/hashmap_internal.h"

namespace leveldb {

/* layout definition */
// TOID_DECLARE(struct buckets, HASHMAP_ATOMIC_TYPE_OFFSET + 1);
// TOID_DECLARE(struct entry, HASHMAP_ATOMIC_TYPE_OFFSET + 2);

enum hashmap_cmd {
	HASHMAP_CMD_REBUILD,
	HASHMAP_CMD_DEBUG,
};

struct entry {
	// uint64_t key;
	// PMEMoid value;
  PMEMoid key;
	uint8_t key_len;
	void* key_ptr;
	// TEST:
	char* buffer_ptr;

	/* list pointer */
	POBJ_LIST_ENTRY(struct entry) list;
  POBJ_LIST_ENTRY(struct entry) iterator;
};

struct entry_args {
	// uint64_t key;
	// PMEMoid value;
  PMEMoid key;
	uint8_t key_len;
	void* key_ptr;
	// TEST:
	char* buffer_ptr;
};

POBJ_LIST_HEAD(entries_head, struct entry);
struct buckets {
	/* number of buckets */
	size_t nbuckets;
	/* array_size of buckets */
	size_t buckets_size[];
	/* array of lists */
	struct entries_head bucket[];
};

POBJ_LIST_HEAD(iterator_head, struct entry);
struct hashmap_atomic {
	/* random number generator seed */
	uint32_t seed;

	/* hash function coefficients */
	uint32_t hash_fun_a;
	uint32_t hash_fun_b;
	uint64_t hash_fun_p;

	/* number of values inserted */
	uint64_t count;
	/* whether "count" should be updated */
	uint32_t count_dirty;

	/* buckets */
	TOID(struct buckets) buckets;
	/* buckets, used during rehashing, null otherwise */
	TOID(struct buckets) buckets_tmp;

  /* TEST: list pointer for ordered iterator */
  // POBJ_LIST_ENTRY(struct entry) iterator;
  struct iterator_head entries;
};

/*
 * create_entry -- entry initializer
 */
static int
create_entry(PMEMobjpool *pop, void *ptr, void *arg)
{
	struct entry *e = (struct entry *)ptr;
	struct entry_args *args = (struct entry_args *)arg;

	e->key = args->key;
  e->key_len = args->key_len;
  e->key_ptr = args->key_ptr;
  e->buffer_ptr = args->buffer_ptr;
  

	memset(&e->list, 0, sizeof(e->list));

	pmemobj_persist(pop, e, sizeof(*e));

	return 0;
}

/*
 * create_buckets -- buckets initializer
 */
static int
create_buckets(PMEMobjpool *pop, void *ptr, void *arg)
{
	struct buckets *b = (struct buckets *)ptr;

	b->nbuckets = *((size_t *)arg);
	// TEST:
	// pmemobj_memset_persist(pop, &b->buckets_size, 0, 
	// 		b->nbuckets * sizeof(b->buckets_size[0]));

	pmemobj_memset_persist(pop, &b->bucket, 0,
			b->nbuckets * sizeof(b->bucket[0]));
	pmemobj_persist(pop, &b->nbuckets, sizeof(b->nbuckets));

	return 0;
}

/*
 * create_hashmap -- hashmap initializer
 */
static void
create_hashmap(PMEMobjpool *pop, TOID(struct hashmap_atomic) hashmap,
		uint32_t seed)
{
	D_RW(hashmap)->seed = seed;

	do {
		D_RW(hashmap)->hash_fun_a = (uint32_t)rand();
	} while (D_RW(hashmap)->hash_fun_a == 0);
	D_RW(hashmap)->hash_fun_b = (uint32_t)rand();
	D_RW(hashmap)->hash_fun_p = HASH_FUNC_COEFF_P;

	size_t len = INIT_BUCKETS_NUM;
	size_t sz = sizeof(struct buckets) +
			len * sizeof(struct entries_head);


	if (POBJ_ALLOC(pop, &D_RW(hashmap)->buckets, struct buckets, sz,
			create_buckets, &len)) {
		fprintf(stderr, "root alloc failed: %s\n", pmemobj_errormsg());
		abort();
	}

	// TEST: iniailize array_size
	// for (int i=0; i<len; i++) {
	// 	// printf("i %d\n", i);
	// 	D_RW(D_RW(hashmap)->buckets)->buckets_size[i] = 0;
	// }

	pmemobj_persist(pop, D_RW(hashmap), sizeof(*D_RW(hashmap)));
}

/*
 * hash -- the simplest hashing function,
 * see https://en.wikipedia.org/wiki/Universal_hashing#Hashing_integers
 */
static uint64_t
hash(const TOID(struct hashmap_atomic) *hashmap,
		const TOID(struct buckets) *buckets,
	// uint64_t value)
	char* value, uint8_t value_len)
{
  std::string key_str;
  key_str.append(value, value_len-NUM_OF_TAG_BYTES);
  uint64_t v = std::atoi(key_str.c_str());
  
	uint32_t a = D_RO(*hashmap)->hash_fun_a;
	uint32_t b = D_RO(*hashmap)->hash_fun_b;
	uint64_t p = D_RO(*hashmap)->hash_fun_p;
	size_t len = D_RO(*buckets)->nbuckets;

	return ((a * v + b) % p) % len;
}

/*
 * hm_atomic_rebuild_finish -- finishes rebuild, assumes buckets_tmp is not null
 */
static void
hm_atomic_rebuild_finish(PMEMobjpool *pop, TOID(struct hashmap_atomic) hashmap)
{
	TOID(struct buckets) cur = D_RO(hashmap)->buckets;
	TOID(struct buckets) tmp = D_RO(hashmap)->buckets_tmp;

	for (size_t i = 0; i < D_RO(cur)->nbuckets; ++i) {
		while (!POBJ_LIST_EMPTY(&D_RO(cur)->bucket[i])) {
			TOID(struct entry) en =
					POBJ_LIST_FIRST(&D_RO(cur)->bucket[i]);
			// uint64_t h = hash(&hashmap, &tmp, D_RO(en)->key);
      char* key = (char *)pmemobj_direct(D_RO(en)->key);
			uint64_t h = hash(&hashmap, &tmp, key, D_RO(en)->key_len);

			if (POBJ_LIST_MOVE_ELEMENT_HEAD(pop,
					&D_RW(cur)->bucket[i],
					&D_RW(tmp)->bucket[h],
					en, list, list)) {
				fprintf(stderr, "move failed: %s\n",
						pmemobj_errormsg());
				abort();
			}
		}
	}

	POBJ_FREE(&D_RO(hashmap)->buckets);

	D_RW(hashmap)->buckets = D_RO(hashmap)->buckets_tmp;
	pmemobj_persist(pop, &D_RW(hashmap)->buckets,
			sizeof(D_RW(hashmap)->buckets));

	/*
	 * We have to set offset manually instead of substituting OID_NULL,
	 * because we won't be able to recover easily if crash happens after
	 * pool_uuid_lo, but before offset is set. Another reason why everyone
	 * should use transaction API.
	 * See recovery process in hm_init and TOID_IS_NULL macro definition.
	 */
	D_RW(hashmap)->buckets_tmp.oid.off = 0;
	pmemobj_persist(pop, &D_RW(hashmap)->buckets_tmp,
			sizeof(D_RW(hashmap)->buckets_tmp));
}

/*
 * hm_atomic_rebuild -- rebuilds the hashmap with a new number of buckets
 */
static void
hm_atomic_rebuild(PMEMobjpool *pop, TOID(struct hashmap_atomic) hashmap,
		size_t new_len)
{
	printf("rebuild\n");
	if (new_len == 0)
		new_len = D_RO(D_RO(hashmap)->buckets)->nbuckets;

	size_t sz = sizeof(struct buckets) +
			new_len * sizeof(struct entries_head);

	POBJ_ALLOC(pop, &D_RW(hashmap)->buckets_tmp, struct buckets, sz,
			create_buckets, &new_len);
	if (TOID_IS_NULL(D_RO(hashmap)->buckets_tmp)) {
		fprintf(stderr,
			"failed to allocate temporary space of size: %zu"
			", %s\n",
			new_len, pmemobj_errormsg());
		return;
	}

	hm_atomic_rebuild_finish(pop, hashmap);
}

/*
 * hm_atomic_insert -- inserts specified value into the hashmap,
 * returns:
 * - 0 if successful,
 * - 1 if value already existed,
 * - -1 if something bad happened
 */
int
hm_atomic_insert(PMEMobjpool *pop, TOID(struct hashmap_atomic) hashmap,
		char* key, char* buffer_ptr, int key_len)
{
	TOID(struct buckets) buckets = D_RO(hashmap)->buckets;
	TOID(struct entry) var;

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
  // printf("1\n");
	uint64_t h = hash(&hashmap, &buckets, key, key_len);
	int num = 0;
	std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();
	std::cout << "hash = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";
	begin = std::chrono::steady_clock::now();
	// int num =	D_RO(buckets)->buckets_size[h];
  // printf("2\n");

  // FIXME: Count same bucket's entries
	POBJ_LIST_FOREACH(var, &D_RO(buckets)->bucket[h], list) {
		// if (D_RO(var)->key == key)
		// 	return 1;
		num++;
	}
	end= std::chrono::steady_clock::now();
	std::cout << "num_foreach = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";
	begin = std::chrono::steady_clock::now();

  // TEST:
	// D_RW(hashmap)->count_dirty = 1;
	// pmemobj_persist(pop, &D_RW(hashmap)->count_dirty,
	// 		sizeof(D_RW(hashmap)->count_dirty));

  // printf("3\n");
	struct entry_args args;
	// args.key = key;
	// args.value = value;
  
  TX_BEGIN(pop) {
    args.key = pmemobj_tx_zalloc(key_len, 501);
    pmemobj_memcpy_persist(pop, pmemobj_direct(args.key), key, key_len);
  } TX_ONABORT {
    return -1;
  } TX_END
  args.key_len = key_len;
  args.key_ptr = nullptr; // default
  args.buffer_ptr = buffer_ptr;
	end= std::chrono::steady_clock::now();
	std::cout << "alloc_copy = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";
	begin = std::chrono::steady_clock::now();
	
  // printf("4\n");
  // For specific bucket
	PMEMoid oid = POBJ_LIST_INSERT_NEW_HEAD(pop,
			&D_RW(buckets)->bucket[h],
			list, sizeof(struct entry), create_entry, &args);
	if (OID_IS_NULL(oid)) {
		fprintf(stderr, "failed to allocate entry1: %s\n",
			pmemobj_errormsg());
		return -1;
	}
	end= std::chrono::steady_clock::now();
	std::cout << "hash_list = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";
	begin = std::chrono::steady_clock::now();
	
  // printf("5\n");
  // For total entries
  PMEMoid oid2 = POBJ_LIST_INSERT_NEW_TAIL(pop,
			&D_RW(hashmap)->entries ,
			iterator, sizeof(struct entry), create_entry, &args);
  if (OID_IS_NULL(oid2)) {
		fprintf(stderr, "failed to allocate entry2: %s\n",
			pmemobj_errormsg());
		return -1;
	}
	end= std::chrono::steady_clock::now();
	std::cout << "iterator = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";
	begin = std::chrono::steady_clock::now();
	

  // printf("6\n");
	D_RW(hashmap)->count++;
	// pmemobj_persist(pop, &D_RW(hashmap)->count,
	// 		sizeof(D_RW(hashmap)->count));

  // TEST:
	// D_RW(hashmap)->count_dirty = 0;
	// pmemobj_persist(pop, &D_RW(hashmap)->count_dirty,
	// 		sizeof(D_RW(hashmap)->count_dirty));

  // printf("7\n");
	num++;
	// printf("num %d]\n", num);
	if (num > MAX_HASHSET_THRESHOLD ||
			(num > MIN_HASHSET_THRESHOLD &&
			D_RO(hashmap)->count > 2 * D_RO(buckets)->nbuckets))
		hm_atomic_rebuild(pop, hashmap, D_RW(buckets)->nbuckets * 2);

	end= std::chrono::steady_clock::now();
	std::cout << "rebuild checking = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";
	
  // printf("8\n");
	return 0;
}

/*
 *  hm_atomic_insert_by_ptr -- inserts a specifics key pointer into the map 
 */
int
hm_atomic_insert_by_ptr(PMEMobjpool* pop, TOID(struct hashmap_atomic) hashmap,
	void* key_ptr, char* buffer_ptr, int key_len)
{
  TOID(struct buckets) buckets = D_RO(hashmap)->buckets;
	TOID(struct entry) var;


	uint64_t h = hash(&hashmap, &buckets, (char *)key_ptr, key_len);
	int num = 0;

  // FIXME: Count same bucket's entries
	POBJ_LIST_FOREACH(var, &D_RO(buckets)->bucket[h], list) {
		// if (D_RO(var)->key == key)
		// 	return 1;
		num++;
	}

  struct entry_args args;
	// args.key = key;
	// args.value = value;
  
  args.key = OID_NULL;  
  args.key_len = key_len;
  args.key_ptr = key_ptr; // NOTE:
  args.buffer_ptr = buffer_ptr;

	PMEMoid oid = POBJ_LIST_INSERT_NEW_HEAD(pop,
			&D_RW(buckets)->bucket[h],
			list, sizeof(struct entry), create_entry, &args);
	if (OID_IS_NULL(oid)) {
		fprintf(stderr, "failed to allocate entry1: %s\n",
			pmemobj_errormsg());
		return -1;
	}
  PMEMoid oid2 = POBJ_LIST_INSERT_NEW_TAIL(pop,
			&D_RW(hashmap)->entries ,
			iterator, sizeof(struct entry), create_entry, &args);
  if (OID_IS_NULL(oid2)) {
		fprintf(stderr, "failed to allocate entry2: %s\n",
			pmemobj_errormsg());
		return -1;
	}

	D_RW(hashmap)->count++;
	pmemobj_persist(pop, &D_RW(hashmap)->count,
			sizeof(D_RW(hashmap)->count));

  // TEST:
	// D_RW(hashmap)->count_dirty = 0;
	// pmemobj_persist(pop, &D_RW(hashmap)->count_dirty,
	// 		sizeof(D_RW(hashmap)->count_dirty));

	num++;
	if (num > MAX_HASHSET_THRESHOLD ||
			(num > MIN_HASHSET_THRESHOLD &&
			D_RO(hashmap)->count > 2 * D_RO(buckets)->nbuckets))
		hm_atomic_rebuild(pop, hashmap, D_RW(buckets)->nbuckets * 2);

	return 0;
}

/*
 * hm_atomic_remove -- removes specified value from the hashmap,
 * returns:
 * - 0 if successful,
 * - 1 if value didn't exist or if something bad happened
 */
int
hm_atomic_remove(PMEMobjpool *pop, TOID(struct hashmap_atomic) hashmap,
		char* key, int key_len)
{
	TOID(struct buckets) buckets = D_RO(hashmap)->buckets;
	TOID(struct entry) var;

	uint64_t h = hash(&hashmap, &buckets, key, key_len);
	POBJ_LIST_FOREACH(var, &D_RW(buckets)->bucket[h], list) {
    void* key_ptr = (void *)pmemobj_direct(D_RO(var)->key);
    if (memcmp(key_ptr, key, key_len) == 0)
		// if (D_RO(var)->key == key)
			break;
	}

	if (TOID_IS_NULL(var))
		return 1;

	// D_RW(hashmap)->count_dirty = 1;
	// pmemobj_persist(pop, &D_RW(hashmap)->count_dirty,
	// 		sizeof(D_RW(hashmap)->count_dirty));

	if (POBJ_LIST_REMOVE_FREE(pop, &D_RW(buckets)->bucket[h],
			var, list)) {
		fprintf(stderr, "list remove failed: %s\n",
			pmemobj_errormsg());
		return 1;
	}

	D_RW(hashmap)->count--;
	pmemobj_persist(pop, &D_RW(hashmap)->count,
			sizeof(D_RW(hashmap)->count));

	// D_RW(hashmap)->count_dirty = 0;
	// pmemobj_persist(pop, &D_RW(hashmap)->count_dirty,
	// 		sizeof(D_RW(hashmap)->count_dirty));

	if (D_RO(hashmap)->count < D_RO(buckets)->nbuckets)
		hm_atomic_rebuild(pop, hashmap, D_RO(buckets)->nbuckets / 2);

	// return D_RO(var)->value;
	return 0;
}

/*
 * hm_atomic_foreach -- prints all values from the hashmap
 */
int
hm_atomic_foreach(PMEMobjpool *pop, TOID(struct hashmap_atomic) hashmap,
	int (*cb)(char* key, char* buffer_ptr, void* key_ptr, int key_len, void *arg), void *arg)
{
	TOID(struct buckets) buckets = D_RO(hashmap)->buckets;
	TOID(struct entry) var;
	int ret = 0;
    // TEST:
	// for (size_t i = 0; i < D_RO(buckets)->nbuckets; ++i)
		// POBJ_LIST_FOREACH(var, &D_RO(buckets)->bucket[i], list) {
		// 	ret = cb(D_RO(var)->key, D_RO(var)->value, arg);
		// 	if (ret)
		// 		return ret;
		// }

    // NOTE: use of LIST
    // TOID(struct entry) first = POBJ_LIST_FIRST(&D_RO(hashmap)->entries);
    // TOID(struct entry) next = POBJ_LIST_NEXT(first, iterator);

	printf("1\n");
		POBJ_LIST_FOREACH(var, &D_RO(hashmap)->entries, iterator) {
	printf("2\n");
			ret = cb((char *)pmemobj_direct(D_RO(var)->key), D_RO(var)->buffer_ptr, 
                D_RO(var)->key_ptr, D_RO(var)->key_len, arg);
	printf("3\n");
			if (ret)
				return ret;
	printf("4\n");
		}

	printf("5\n");
	return 0;
}

/*
 * hm_atomic_debug -- prints complete hashmap state
 */
static void
hm_atomic_debug(PMEMobjpool *pop, TOID(struct hashmap_atomic) hashmap,
		FILE *out)
{
	TOID(struct buckets) buckets = D_RO(hashmap)->buckets;
	TOID(struct entry) var;

	fprintf(out, "a: %u b: %u p: %" PRIu64 "\n", D_RO(hashmap)->hash_fun_a,
		D_RO(hashmap)->hash_fun_b, D_RO(hashmap)->hash_fun_p);
	fprintf(out, "count: %" PRIu64 ", buckets: %zu\n",
		D_RO(hashmap)->count, D_RO(buckets)->nbuckets);

	for (size_t i = 0; i < D_RO(buckets)->nbuckets; ++i) {
		if (POBJ_LIST_EMPTY(&D_RO(buckets)->bucket[i]))
			continue;

		int num = 0;
		fprintf(out, "%zu: ", i);
		POBJ_LIST_FOREACH(var, &D_RO(buckets)->bucket[i], list) {
			fprintf(out, "%" PRIu64 " ", D_RO(var)->key);
			num++;
		}
		fprintf(out, "(%d)\n", num);
	}
}

/*
 * hm_atomic_get -- checks whether specified value is in the hashmap
 */
void*
hm_atomic_get(PMEMobjpool *pop, TOID(struct hashmap_atomic) hashmap,
		char* key, int key_len)
{
	TOID(struct buckets) buckets = D_RO(hashmap)->buckets;
	TOID(struct entry) var;

	uint64_t h = hash(&hashmap, &buckets, key, key_len);

	POBJ_LIST_FOREACH(var, &D_RO(buckets)->bucket[h], list) {
    void* key_ptr = (void *)pmemobj_direct(D_RO(var)->key);
		// if (D_RO(var)->key == key)
    if (memcmp(key_ptr, key, key_len) == 0)
			return D_RO(var)->buffer_ptr;
  }
	return nullptr;
}

/*
 * hm_atomic_lookup -- checks whether specified value is in the hashmap
 */
int
hm_atomic_lookup(PMEMobjpool *pop, TOID(struct hashmap_atomic) hashmap,
		char* key, int key_len)
{
	TOID(struct buckets) buckets = D_RO(hashmap)->buckets;
	TOID(struct entry) var;

	uint64_t h = hash(&hashmap, &buckets, key, key_len);

	POBJ_LIST_FOREACH(var, &D_RO(buckets)->bucket[h], list) {
		void* key_ptr = (void *)pmemobj_direct(D_RO(var)->key);
		// if (D_RO(var)->key == key)
    if (memcmp(key_ptr, key, key_len) == 0)
			return 1;
  }
	return 0;
}

/*
 * hm_atomic_create --  initializes hashmap state, called after pmemobj_create
 */
int
hm_atomic_create(PMEMobjpool *pop, TOID(struct hashmap_atomic) *map, void *arg)
{
	// struct hashmap_args *args = (struct hashmap_args *)arg;
	// uint32_t seed = args ? args->seed : 0;
  uint32_t seed = 0;

	POBJ_ZNEW(pop, map, struct hashmap_atomic);

	create_hashmap(pop, *map, seed);

	return 0;
}

/*
 * hm_atomic_init -- recovers hashmap state, called after pmemobj_open
 */
int
hm_atomic_init(PMEMobjpool *pop, TOID(struct hashmap_atomic) hashmap)
{
	srand(D_RO(hashmap)->seed);

	/* handle rebuild interruption */
	if (!TOID_IS_NULL(D_RO(hashmap)->buckets_tmp)) {
		printf("rebuild, previous attempt crashed\n");
		if (TOID_EQUALS(D_RO(hashmap)->buckets,
				D_RO(hashmap)->buckets_tmp)) {
			/* see comment in hm_rebuild_finish */
			D_RW(hashmap)->buckets_tmp.oid.off = 0;
			pmemobj_persist(pop, &D_RW(hashmap)->buckets_tmp,
					sizeof(D_RW(hashmap)->buckets_tmp));
		} else if (TOID_IS_NULL(D_RW(hashmap)->buckets)) {
			D_RW(hashmap)->buckets = D_RW(hashmap)->buckets_tmp;
			pmemobj_persist(pop, &D_RW(hashmap)->buckets,
					sizeof(D_RW(hashmap)->buckets));
			/* see comment in hm_rebuild_finish */
			D_RW(hashmap)->buckets_tmp.oid.off = 0;
			pmemobj_persist(pop, &D_RW(hashmap)->buckets_tmp,
					sizeof(D_RW(hashmap)->buckets_tmp));
		} else {
			hm_atomic_rebuild_finish(pop, hashmap);
		}
	}

	/* handle insert or remove interruption */
	if (D_RO(hashmap)->count_dirty) {
		printf("count dirty, recalculating\n");
		TOID(struct entry) var;
		TOID(struct buckets) buckets = D_RO(hashmap)->buckets;
		uint64_t cnt = 0;

		for (size_t i = 0; i < D_RO(buckets)->nbuckets; ++i)
			POBJ_LIST_FOREACH(var, &D_RO(buckets)->bucket[i], list)
				cnt++;

		printf("old count: %" PRIu64 ", new count: %" PRIu64 "\n",
			D_RO(hashmap)->count, cnt);
		D_RW(hashmap)->count = cnt;
		pmemobj_persist(pop, &D_RW(hashmap)->count,
				sizeof(D_RW(hashmap)->count));

		D_RW(hashmap)->count_dirty = 0;
		pmemobj_persist(pop, &D_RW(hashmap)->count_dirty,
				sizeof(D_RW(hashmap)->count_dirty));
	}
  
  // NOTE: ClearAll to iterator
  // TOID(struct entry)* var;
  // TOID(struct buckets) buckets = D_RO(hashmap)->buckets;
  // for (size_t i = 0; i < D_RO(buckets)->nbuckets; ++i) {
  //   POBJ_LIST_FOREACH(*var, &D_RW(buckets)->bucket[i], iterator) {
      
  //     POBJ_LIST_REMOVE_FREE(pop, &D_RW(hashmap)->entries, *var, iterator);
  //     // void* key_ptr = (void *)pmemobj_direct(D_RO(var)->key);
  //     // if (memcmp(key_ptr, key, key_len) == 0)
  //     //   // return &(var.oid);
  //     //   return &var->oid;
  //   }
  // }

	return 0;
}

/*
 * hm_atomic_get_prev_OID -- searches for prev OID of the key
 */
PMEMoid*
hm_atomic_get_prev_OID(PMEMobjpool* pop, TOID(struct entry) current_entry)
{	
	PMEMoid* prev = 
  // TOID(struct entry)* prev = 
  const_cast<PMEMoid *>(&POBJ_LIST_PREV(current_entry, iterator).oid);

  if (!OID_IS_NULL(*prev)) {
    return prev;
  } 
  return const_cast<PMEMoid *>(&OID_NULL);
}
/*
 * hm_atomic_get_next_OID -- searches for next OID of the key
 */
PMEMoid*
hm_atomic_get_next_OID(PMEMobjpool* pop, TOID(struct entry) current_entry)
{	
  PMEMoid* next = 
	// TOID(struct entry) next = 
  const_cast<PMEMoid *>(&POBJ_LIST_NEXT(current_entry, iterator).oid);

  if (!OID_IS_NULL(*next)) {
    return next;
  } 
  return const_cast<PMEMoid *>(&OID_NULL);
}
/*
 * hm_atomic_get_first_OID -- searches for OID of first node
 */
PMEMoid*
hm_atomic_get_first_OID(PMEMobjpool* pop, TOID(struct hashmap_atomic) hashmap)
{	
	// PMEMoid* res;
	// // Check whether first-node is valid
	// uint8_t key_len = D_RO(D_RO(map)->next[0])->entry.key_len;
	// if (key_len) {
	// 	res = &(D_RW(map)->next[0].oid);
	// }
	// return res;
  // TOID(struct entry) first = 
  PMEMoid* first = 
  const_cast<PMEMoid *>(&POBJ_LIST_FIRST(&D_RO(hashmap)->entries).oid);
  if (!OID_IS_NULL(*first)) {
    return first;
  } 
  return const_cast<PMEMoid *>(&OID_NULL);
}
/*
 * hm_atomic_get_last_OID -- searches for OID of last node
 */
PMEMoid*
hm_atomic_get_last_OID(PMEMobjpool* pop, TOID(struct hashmap_atomic) hashmap)
{	
	// TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];
	// // Seek non-empty node
	// skiplist_map_get_last_find(pop, map, path);
	// res = &(path[0].oid);
	// return res;
  PMEMoid* last =
  // TOID(struct entry) last = 
  const_cast<PMEMoid *>(&POBJ_LIST_LAST(&D_RO(hashmap)->entries, iterator).oid);
  if (!OID_IS_NULL(*last)) {
    return last;
  } 
  return const_cast<PMEMoid *>(&OID_NULL);
}
/*
 * hm_atomic_seek_OID -- searches for next OID of the key
 */
PMEMoid*
hm_atomic_seek_OID(PMEMobjpool* pop, TOID(struct hashmap_atomic) hashmap,
		char* key, int key_len)
{	
	// TOID(struct entry) next = POBJ_LIST_NEXT(current_entry, iterator);

  // if (!TOID_IS_NULL(next)) {
  //   return &next.oid;
  // } 

  TOID(struct buckets) buckets = D_RO(hashmap)->buckets;
  TOID(struct entry) var;

  uint64_t h = hash(&hashmap, &buckets, key, key_len);

  POBJ_LIST_FOREACH(var, &D_RO(buckets)->bucket[h], list) {
    void* key_ptr = (void *)pmemobj_direct(D_RO(var)->key);
    if (memcmp(key_ptr, key, key_len) == 0) {
		  // return &(var.oid);
      return &var.oid;
		}
  }
  return const_cast<PMEMoid *>(&OID_NULL);
}
/*
 * hm_atomic_check -- checks if specified persistent object is an
 * instance of hashmap
 */
int
hm_atomic_check(PMEMobjpool *pop, TOID(struct hashmap_atomic) hashmap)
{
	return TOID_IS_NULL(hashmap) || !TOID_VALID(hashmap);
}

/*
 * hm_atomic_count -- returns number of elements
 */
size_t
hm_atomic_count(PMEMobjpool *pop, TOID(struct hashmap_atomic) hashmap)
{
	return D_RO(hashmap)->count;
}

/*
 * hm_atomic_cmd -- execute cmd for hashmap
 */
int
hm_atomic_cmd(PMEMobjpool *pop, TOID(struct hashmap_atomic) hashmap,
		unsigned cmd, uint64_t arg)
{
	switch (cmd) {
		case HASHMAP_CMD_REBUILD:
			hm_atomic_rebuild(pop, hashmap, arg);
			return 0;
		case HASHMAP_CMD_DEBUG:
			if (!arg)
				return -EINVAL;
			hm_atomic_debug(pop, hashmap, (FILE *)arg);
			return 0;
		default:
			return -EINVAL;
	}
}

} // namespace leveldb