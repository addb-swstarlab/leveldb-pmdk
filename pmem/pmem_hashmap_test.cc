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
// #include "pmem/layout.h"
// #include "pmem/ds/hashmap_atomic.h"
#include "pmem/pmem_skiplist.h"




using namespace std;
namespace pobj = pmem::obj;

namespace leveldb {


class PmemHashmapTest { };

inline bool
file_exists (const std::string &name)
{
	std::ifstream f (name.c_str ());
	return f.good ();
}

// struct entry {
// 	uint64_t key;
// 	PMEMoid value;

// 	/* list pointer */
// 	POBJ_LIST_ENTRY(struct entry) list;
// };

// struct entry_args {
// 	uint64_t key;
// 	PMEMoid value;
// };

// POBJ_LIST_HEAD(entries_head, struct entry);
// struct buckets {
// 	/* number of buckets */
// 	size_t nbuckets;
// 	/* array of lists */
// 	struct entries_head bucket[];
// };

// struct hashmap_atomic {
// 	/* random number generator seed */
// 	uint32_t seed;

// 	/* hash function coefficients */
// 	uint32_t hash_fun_a;
// 	uint32_t hash_fun_b;
// 	uint64_t hash_fun_p;

// 	/* number of values inserted */
// 	uint64_t count;
// 	/* whether "count" should be updated */
// 	uint32_t count_dirty;

// 	/* buckets */
// 	TOID(struct buckets) buckets;
// 	/* buckets, used during rehashing, null otherwise */
// 	TOID(struct buckets) buckets_tmp;
// };

TEST (PmemHashmapTest, HashMap) {
  
  printf("# Start Hashmap\n");

  PmemHashmap* pmem_hashmap = new PmemHashmap(HASHMAP_PATH);

  // for (int i=0; i<10; i++) {
  printf("## Start Insertion\n");

    char* tmp = "12345";
    TX_BEGIN(pmem_hashmap->GetPool()) {
      PMEMoid value1 = pmemobj_tx_zalloc(5, 501);
      PMEMoid value2 = pmemobj_tx_zalloc(5, 501);
      PMEMoid value3 = pmemobj_tx_zalloc(5, 501);
      PMEMoid value4 = pmemobj_tx_zalloc(5, 501);
      PMEMoid value5 = pmemobj_tx_zalloc(5, 501);

      pmemobj_memcpy_persist(pmem_hashmap->GetPool(), pmemobj_direct(value1), tmp, 5);
      pmemobj_memcpy_persist(pmem_hashmap->GetPool(), pmemobj_direct(value2), tmp, 5);
      pmemobj_memcpy_persist(pmem_hashmap->GetPool(), pmemobj_direct(value3), tmp, 5);
      pmemobj_memcpy_persist(pmem_hashmap->GetPool(), pmemobj_direct(value4), tmp, 5);
      pmemobj_memcpy_persist(pmem_hashmap->GetPool(), pmemobj_direct(value5), tmp, 5);

      pmem_hashmap->Insert(0, 0, value1);
      pmem_hashmap->Insert(0, 1, value2);
      pmem_hashmap->Insert(0, 2, value3);
      pmem_hashmap->Insert(0, 4, value3);
      pmem_hashmap->Insert(0, 5, value3);
    } TX_END

  printf("## End Insertion\n");

  printf("## Start PrintAll\n");
  pmem_hashmap->PrintAll(0);

  printf("## End PrintAll\n");

  // }

  // hm_atomic_create(hashmap_pool.get_handle(), &(root_hashmap_[0].head), nullptr);

  
	printf("# End Hashmap\n");
}

} // namespace leveldb

/* Main */
int main(int argc, char** argv) {
  return leveldb::test::RunAllTests();
}
/* End Main */
