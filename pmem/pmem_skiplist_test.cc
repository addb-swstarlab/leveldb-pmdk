/*
Copyright (c) 2018 Intel Corporation

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
 
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 
SPDX-License-Identifier: BSD-3-Clause
*/

// For TEST tool
#include "util/logging.h"
#include "util/testharness.h"

// For this test
#include <iostream>
#include <fstream> //file_exists
#include <chrono>
#include "pmem/layout.h"
// #include "pmem/map/map.h"
#include "pmem/map/map_skiplist.h"
#include "pmem/map/hashmap.h"
// #include <errno.h>
// #include "pmem/pmem_skiplist.h"

#define KEY_CONCAT(x) key## x
#define VALUE_CONCAT(x) value ## x


using namespace std;

namespace leveldb {


class PmemSkiplistManagerTest { };

inline bool
file_exists (const std::string &name)
{
	std::ifstream f (name.c_str ());
	return f.good ();
}

static int
hashmap_print(char *key, char *value, void *arg)
{
	if (strcmp(key, "") != 0 && strcmp(value, "") != 0)
		printf("[print] [key %d]:'%s', [value %d]:'%s'\n", strlen(key), key, strlen(value), value);
		// printf("[print] [key %d]:'%s', [value %d]:'%s'\n", strlen(key), strlen(value), key, value);
		// printf("[print] [key]:'%s', [value]:'%s'\n", key, value);
	// if (strlen(key) != 0 && strlen(value) != 0)
	// 	printf("[print] [key %d]:'%s', [value %d]:'%s'\n", strlen(key), strlen(value), key, value);
	delete[] key;
	delete[] value;
	return 0;
}

TEST (PmemSkiplistManagerTest, Skiplist_manager) {
	// PmemSkiplist *pmemSkiplist = new PmemSkiplist();

	POBJ_LAYOUT_BEGIN(root_skiplist_manager);
	POBJ_LAYOUT_ROOT(root_skiplist_manager, struct root_skiplist_manager);
	POBJ_LAYOUT_TOID(root_skiplist_manager, struct root_skiplist_map);
	// POBJ_LAYOUT_TOID(root_skiplist_manager, struct skiplist_map_node);
	POBJ_LAYOUT_END(root_skiplist_manager);

	// struct skiplist_map_entry {
  //   uint64_t key;
  //   PMEMoid value;
  // };

  // struct skiplist_map_node {
  //   TOID(struct skiplist_map_node) next[12];
  //   struct skiplist_map_entry entry;
  // };

	struct root_skiplist_map {
 		TOID(struct map) map;
	};

	struct root_skiplist_manager {
		int count;
		PMEMoid skiplists;
		// TOID(struct root_skiplist_map) *skiplists;
		// TOID(struct root_skiplist_map) skiplists[];
		// struct root_skiplist_map *skiplists;
        // Offsets
	};


	static PMEMobjpool *pool;
	static struct map_ctx *mapc;
	const struct map_ops *ops = MAP_SKIPLIST;

	static TOID(struct root_skiplist_manager) root;
  static struct root_skiplist_map *skiplists;
  // static TOID(struct root_skiplist_map) *skiplists;
	static TOID(struct map) *map;
	// static TOID(struct map) map[SKIPLIST_MANAGER_LIST_SIZE];



	cout << "# Start Skiplist_manager" << endl;
	// NOTE: Create pool
	if (!file_exists(SKIPLIST_MANAGER_PATH)) {
        /* Initialize pool & map_ctx */
		pool = pmemobj_create(SKIPLIST_MANAGER_PATH, 
								POBJ_LAYOUT_NAME(root_skiplist_manager), 
								(unsigned long) SKIPLIST_MANAGER_POOL_SIZE, 0666); 
		if (pool == NULL) {
			fprintf(stderr, "failed to create pool: %s\n",
					pmemobj_errormsg());
			exit(1);
		}
		mapc = map_ctx_init(ops, pool);
		if (!mapc) {
			pmemobj_close(pool);
			perror("map_ctx_init");
			exit(1);
		}		
		struct hashmap_args args; // empty
		/* 
		 * 1) Get root and make root ptr.
		 *    skiplists' PMEMoid is oid of overall skiplists
		 */
		root = POBJ_ROOT(pool, struct root_skiplist_manager);
		struct root_skiplist_manager *root_ptr = D_RW(root);
		/*
		 * 2) Initialize root structure
		 *    It includes malloc overall skiplists
		 */
		root_ptr->count = SKIPLIST_MANAGER_LIST_SIZE;
		// printf("%d\n", root_ptr->count);
		
		// TX_BEGIN(pool) {
		// 	for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
		// 		root_ptr->skiplists[i] = TX_ZNEW(struct root_skiplist_map);
		// 	}
		// } TX_ONABORT {
		// 	printf("abort\n");
		// } TX_END
		// Error. Need to znew struct root_skiplist_map
		TX_BEGIN(pool) {
			root_ptr->skiplists = pmemobj_tx_zalloc(
														// sizeof(root_skiplist_map), 
														sizeof(root_skiplist_map) * SKIPLIST_MANAGER_LIST_SIZE, 
														SKIPLIST_MAP_TYPE_OFFSET + 1000);
		} TX_ONABORT {
			printf("abort bye..\n");
		} TX_END
		/* 
		 * 3) map_create for all indices
		 */
		skiplists = (struct root_skiplist_map *)pmemobj_direct(root_ptr->skiplists);
		map = (TOID(struct map) *) malloc(sizeof(TOID(struct map)) * SKIPLIST_MANAGER_LIST_SIZE);
		
		// int ret = 0;
		// TX_BEGIN(pool) {
		// 	for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
		// 		pmemobj_tx_add_range_direct(skiplists[i], sizeof(skiplists[i]));
		// 		skiplists[i] = TX_ZNEW(struct root_skiplist_map);
		// 	}
		// } TX_ONABORT {
		// 	ret = 1;
		// } TX_END
		// printf("[5]\n");
		/* NOTE: create */
		for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
			int res = map_create(mapc, &(skiplists[i].map), i, &args); // [ERROR] D_RW need TOID
			if (res) printf("[CREATE ERROR %d] %d\n",i ,res);
			else printf("[CREATE SUCCESS %d] %d\n",i ,res);	
			map[i] = skiplists[i].map;
		}
		/* NOTE: bulk insert */
		for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
			// for (int j=0; j<NUM_OF_PRE_ALLOC_NODE; j++) {
			chrono::steady_clock::time_point begin, end;

			for (int j=0; j<5; j++) {
			// for (int j=0; j<SKIPLIST_BULK_INSERT_NUM; j++) {
				char key[] = "key-";
				stringstream ss_key, ss_value;
				ss_key << key << i << "-" << j;
				char value[] = "value-";
				// make max 100bytes
				ss_value << value << i << "-" << j 
				<< ".......................................................................................";
				// << "..............................................";
				// printf("'%s'-'%s'\n", (char *)ss_key.str().c_str(), (char *)ss_value.str().c_str());
			begin = std::chrono::steady_clock::now();
				int res = map_insert(mapc, map[i], 
									(char *)ss_key.str().c_str(), (char *)ss_value.str().c_str(), 
									ss_key.str().size(), ss_value.str().size(), i);
			end = std::chrono::steady_clock::now();
			std::cout << "insert"<< i <<" = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";
				if(res) { fprintf(stderr, "[ERROR] insert %d-%d\n", i, j);  abort();} 
				// else if (!res && (j%10==0)) printf("insert %d] success\n", j);
			}

			printf("Insert %d] End\n", i);
		}

	} 
	// NOTE: Open
	else {
		pool = pmemobj_open(SKIPLIST_MANAGER_PATH, 
                            POBJ_LAYOUT_NAME(root_skiplist_manager));
		if (pool == NULL) {
			fprintf(stderr, "failed to open pool: %s\n",
					pmemobj_errormsg());
			exit(1);
		}
		mapc = map_ctx_init(ops, pool);
		if (!mapc) {
			pmemobj_close(pool);
			perror("map_ctx_init");
			exit(1);
		}
		root = POBJ_ROOT(pool, struct root_skiplist_manager);
		struct root_skiplist_manager *root_ptr = D_RW(root);
		skiplists = (struct root_skiplist_map *)pmemobj_direct(root_ptr->skiplists);
		map = (TOID(struct map) *) malloc(
											sizeof(TOID(struct map)) * SKIPLIST_MANAGER_LIST_SIZE);
		/* NOTE: foreach test */
		for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE-1; i++) {
				map[i] = skiplists[i].map;
				// char *key = "key-0-5";
				// char *key = "key-0-30400";
				// char *get = map_get(mapc, map[i], key);
				// if (strlen(get) != 0) printf("get %d] key:'%s',value:'%s'\n",i, key, get);
				// else printf("get] cannot get key %d:'%s'\n",i, key);
				// printf("Get-End\n");
				// delete get;

				printf("[%d]\n",i);
				// map_clear(mapc, map[i]);
				map_foreach(mapc, map[i], hashmap_print, NULL);
				printf("\n");
		}
		// TOID(struct map) *prev, *curr;
		// printf("start\n");
		// map_get_next_TOID(mapc, map[8], "key-8-1", prev, curr);
		// printf("end\n");

		/* TEST: swap oid test */
		// struct hashmap_args args; // empty
		// int res = map_create(mapc, &(skiplists[9].map), 9, args); // [ERROR] D_RW need TOID
		// if (res) printf("[CREATE ERROR %d] %d\n",9 ,res);
		// else printf("[CREATE SUCCESS %d] %d\n",9 ,res);	
		// map[9] = skiplists[9].map;
		// chrono::steady_clock::time_point begin, end;
		// begin = std::chrono::steady_clock::now();
		// for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
		// 		printf("[%d]\n",i);
		// 		if (i != 9) {
		// 			chrono::steady_clock::time_point begin, end;

			
		// 			PMEMoid *oid = const_cast<PMEMoid *>(map_get_first_OID(mapc, map[i]));
		// 	begin = std::chrono::steady_clock::now();
		// 			map_insert_by_oid(mapc, map[9], 9, oid);
		// 	end = std::chrono::steady_clock::now();
		// 	std::cout << "insert by oid"<< i <<" = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";
		// 		}
		// 		// printf("\n");
		// }
		// begin = std::chrono::steady_clock::now();
		// map_insert_null_node(mapc, map[9], 9);
		// 	end = std::chrono::steady_clock::now();
		// 	std::cout << "insert null node"<< " = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";
		// for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
		// 	printf("[%d]\n",i);
		// 		map_foreach(mapc, map[i], hashmap_print, NULL);
		// }

	// for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
	// 		map[i] = D_RW(root_ptr->skiplists[i])->map;
	// 		char *key = "123";
	// 		char *get = map_get(mapc, map[i], key);
	// 		if (strlen(get) != 0) printf("get %d] key:'%s',value:'%s'\n",i, key, get);
	// 		else printf("get] cannot get key %d:'%s'\n",i, key);
	// }

	// 	/* insert */
	// 	// int res;
	// 	// res = map_insert(mapc, map, key, value); 
	// 	// if (res) {
	// 	// 	fprintf(stderr, "[ERROR] insert\n");
	// 	// } else {
	// 	// 	printf("insert] success\n");			
	// 	// }

	// 	/* blk-insert */
	// 	// {
	// 	// 	char *key1="789", *key2="123", *key3="456", *key4="45";
	// 	// 	char *value1="value789", *value2="value123",
	// 	// 		*value3="value456", *value4="value45";
	// 	// 	map_insert(mapc, map, KEY_CONCAT(1),VALUE_CONCAT(1));
	// 	// 	map_insert(mapc, map, KEY_CONCAT(2),VALUE_CONCAT(2));
	// 	// 	map_insert(mapc, map, KEY_CONCAT(3),VALUE_CONCAT(3));
	// 	// 	map_insert(mapc, map, KEY_CONCAT(4),VALUE_CONCAT(4));
	// 	// 	printf("insert] success\n");			
	// 	// }

	// 	/* lookup */
	// 	// int r = map_lookup(mapc, map, key); // exist:=1  / Not exist:=0
	// 	// printf("lookup] %d\n", r);

	// 	/* get */
	// 	// char *get = map_get(mapc, map, key);
	// 	// if (strlen(get) != 0) printf("get] key:'%s',value:'%s'\n", key, get);
	// 	// else printf("get] cannot get key:'%s'\n",key);

	// 	// /* remove */
	// 	// char *remove = map_remove(mapc, map, key);
	// 	// if (strlen(remove) != 0) printf("remove] key:'%s',value:'%s'\n", key, remove);
	// 	// else printf("remove] cannot remove key:'%s'\n",key);

	// 	/* remove_free */
	// 	// int remove_free = map_remove_free(mapc, map, key);
	// 	// if (remove_free) printf("remove_free %d] key:'%s'\n", remove_free, key);
	// 	// else printf("remove_free] cannot remove key:'%s'\n",key);

	// 	/* clear */
	// 	// printf("Clear]\n");
	// 	// map_clear(mapc, map);

	// 	/* destroy */
	// 	// int destroy = map_destroy(mapc, &map); // SUCCESS:=0 / FAILED:=1
	// 	// printf("destroy] %s\n", destroy ? "FAILED" : "SUCCESS");

	// 	/* print all */
	// 	if (mapc->ops->count)
	// 		printf("count: %zu\n", map_count(mapc, map));
	// 	map_foreach(mapc, map, hashmap_print, NULL);
	// 	printf("\n");

	// 	/* is_empty */
	// 	// printf("[is empty] %d\n",map_is_empty(mapc, map)); // empty:=1/Not-empty:=0

	// 	/* check persistent */
	// 	// printf("[p-check] %d\n", map_check(mapc, map));		 // Not-p:=1/p:=0

	} 
    
	// cout << "# Center " << endl;
	// ASSERT_EQ(0 , strcmp("mutex1", "mutex1"));

	/* Close persistent pool */
	printf("# End Skiplist_map\n");
	pmemobj_close(pool);
}

} // namespace leveldb

/* Main */
int main(int argc, char** argv) {
  return leveldb::test::RunAllTests();
}
/* End Main */
