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
#include "pmem/pmem_map.h"
#include "pmem/layout.h"
#include "pmem/map/map.h"
#include "pmem/map/map_skiplist.h"
#include "pmem/map/hashmap.h"
// #include <errno.h>

#define KEY_CONCAT(x) key## x
#define VALUE_CONCAT(x) value ## x

namespace leveldb {


class PmemMapTest { };

inline bool
file_exists (const std::string &name)
{
	std::ifstream f (name.c_str ());
	return f.good ();
}

/* Main */
// TEST (PmemMapTest, PmapHelloWorld) {
// 	// struct root_pmap {
//   // 	char buf[BUF_SIZE];
// 	// };
// 	PMEMobjpool *pool;
// 	// PMEMoid root;
// 	cout << "# Start " << endl;
// 	// Prepare the input to store into persistent memory
	
// 	if (!file_exists(PMAP_PATH)) {
// 		pool = pmemobj_create(PMAP_PATH, POBJ_LAYOUT_NAME(root_skiplist_map), 
// 													PMAP_SIZE, 0666);
// 		// 							  S_IRUSR | S_IWUSR
// 		if (pool == NULL) {
// 			exit(1);
// 		}
	
// 		TOID(struct root_pmap) root;
		
// 		root = POBJ_ROOT(pool, struct root_pmap);

// 		char *buf = "Hello world!";

// 		TX_BEGIN(pool) {
// 			TX_MEMCPY(D_RW(root)->buf, buf, strlen(buf));
// 		} TX_END
// 		// root = pmemobj_root(pool, sizeof(struct root_pmap));
// 		// struct root_pmap *rootptr = (root_pmap*)pmemobj_direct(root);
// 		// rootptr->len = strlen(buf);
// 		// pmemobj_persist(pool, &rootptr->len, sizeof(rootptr->len));
// 		// pmemobj_memcpy_persist(pool, rootptr->buf, buf, rootptr->len);

//     // printf("[DEBUG] Result: %s\n", rootptr->buf);
// 		// int check = pmemobj_check(PMAP_PATH, POOL1);
//     // printf("[DEBUG] Check: %d\n", check);

// 		printf("# End \n");
		
// 	} 
// 	else {
// 		pool = pmemobj_open(PMAP_PATH, POOL1);

// 		TOID(struct root_pmap) root;
		
// 		root = POBJ_ROOT(pool, struct root_pmap);
// 		TX_BEGIN(pool) {
// 			printf("[Read]%s\n", D_RO(root)->buf);		
// 		} TX_END
		
// 	}

  
// 	// cout << "# Center " << endl;
// 	// ASSERT_EQ(0 , strcmp("mutex1", "mutex1"));

// 	/* Cleanup */
// 	// cout << "# End " << endl;
//   // pobj::transaction::exec_tx( pool1, [&] {
//   //   pobj::delete_persistent<PmemMap>(ptr->pmap_ptr);
// 	// });
// 	/* Close persistent pool */
// 	pmemobj_close(pool);
// }


static int
hashmap_print(char *key, char *value, void *arg)
{
	printf("[print] key:'%s', value:'%s'\n", key, value);
	return 0;
}

TEST (PmemMapTest, Skiplist_map) {

	POBJ_LAYOUT_BEGIN(root_skiplist_map);
	POBJ_LAYOUT_ROOT(root_skiplist_map, struct root_skiplist_map);
	POBJ_LAYOUT_END(root_skiplist_map);
	struct root_skiplist_map {
 		TOID(struct map) map;
	};
	static PMEMobjpool *pool;
	static struct map_ctx *mapc;
	static TOID(struct root_skiplist_map) root;
	static TOID(struct map) map;
	const struct map_ops *ops = MAP_SKIPLIST;

	cout << "# Start Skiplist_map" << endl;
	// Prepare the input to store into persistent memory
	
	if (!file_exists(SKIPLIST_PATH)) {
		pool = pmemobj_create(SKIPLIST_PATH, 
								POBJ_LAYOUT_NAME(root_skiplist_map), 
								SKIPLIST_POOL_SIZE, 0666); // S_IRUSR | S_IWUSR
		if (pool == NULL) {
			fprintf(stderr, "failed to create pool: %s\n",
					pmemobj_errormsg());
			exit(1);
		}

		struct hashmap_args args;
		// args.seed = (uint32_t)time(NULL);
		// srand(args.seed);

		mapc = map_ctx_init(ops, pool);
		if (!mapc) {
			pmemobj_close(pool);
			perror("map_ctx_init");
			exit(1);
		}

		root = POBJ_ROOT(pool, struct root_skiplist_map);

		int res = map_create(mapc, &D_RW(root)->map, 0, &args);

		if (res) printf("[CREATE ERROR] %d\n",res);
		else printf("[CREATE SUCCESS] %d\n",res);	
		map = D_RO(root)->map;

	} else {
		pool = pmemobj_open(SKIPLIST_PATH, POBJ_LAYOUT_NAME(root_skiplist_map));
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
		root = POBJ_ROOT(pool, struct root_skiplist_map);
		map = D_RW(root)->map;
		// char *test = " 12";
		// char *key = "789";
		// char *value = "value45";
		/* insert */
		// int res;
		// res = map_insert(mapc, map, key, value); 
		// if (res) {
		// 	fprintf(stderr, "[ERROR] insert\n");
		// } else {
		// 	printf("insert] success\n");			
		// }

		/* blk-insert */
		// {
		// 	char *key1="789", *key2="123", *key3="456", *key4="45";
		// 	char *value1="value789", *value2="value123",
		// 		*value3="value456", *value4="value45";
		// 	map_insert(mapc, map, KEY_CONCAT(1),VALUE_CONCAT(1));
		// 	map_insert(mapc, map, KEY_CONCAT(2),VALUE_CONCAT(2));
		// 	map_insert(mapc, map, KEY_CONCAT(3),VALUE_CONCAT(3));
		// 	map_insert(mapc, map, KEY_CONCAT(4),VALUE_CONCAT(4));
		// 	printf("insert] success\n");			
		// }

		/* lookup */
		// int r = map_lookup(mapc, map, key); // exist:=1  / Not exist:=0
		// printf("lookup] %d\n", r);

		/* get */
		// char *get = map_get(mapc, map, key);
		// if (strlen(get) != 0) printf("get] key:'%s',value:'%s'\n", key, get);
		// else printf("get] cannot get key:'%s'\n",key);

		// /* remove */
		// char *remove = map_remove(mapc, map, key);
		// if (strlen(remove) != 0) printf("remove] key:'%s',value:'%s'\n", key, remove);
		// else printf("remove] cannot remove key:'%s'\n",key);

		/* remove_free */
		// int remove_free = map_remove_free(mapc, map, key);
		// if (remove_free) printf("remove_free %d] key:'%s'\n", remove_free, key);
		// else printf("remove_free] cannot remove key:'%s'\n",key);

		/* clear */
		// printf("Clear]\n");
		// map_clear(mapc, map);

		/* destroy */
		// int destroy = map_destroy(mapc, &map); // SUCCESS:=0 / FAILED:=1
		// printf("destroy] %s\n", destroy ? "FAILED" : "SUCCESS");

		/* print all */
		map_foreach(mapc, map, hashmap_print, NULL);
		printf("\n");

		/* is_empty */
		// printf("[is empty] %d\n",map_is_empty(mapc, map)); // empty:=1/Not-empty:=0

		/* check persistent */
		// printf("[p-check] %d\n", map_check(mapc, map));		 // Not-p:=1/p:=0

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
