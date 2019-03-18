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
#include "pmem/pmem_hashmap.h"




using namespace std;
namespace pobj = pmem::obj;

namespace leveldb {

struct entry {
  PMEMoid key;
  uint8_t key_len;
  void* key_ptr;
  char* buffer_ptr;

  /* list pointer */
  POBJ_LIST_ENTRY(struct entry) list;
  POBJ_LIST_ENTRY(struct entry) iterator;
};

class PmemHashmapTest { };

inline bool
file_exists (const std::string &name)
{
	std::ifstream f (name.c_str ());
	return f.good ();
}

TEST (PmemHashmapTest, HashMap) {
  
  printf("# Start Hashmap\n");

  PmemHashmap* pmem_hashmap = new PmemHashmap(HASHMAP_PATH_5);

  // pmem_hashmap->ClearAll();

  // for (int i=0; i<10; i++) {
  // printf("## Start Insertion\n");

  // char* tmp = "1234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345";
  // pmem_hashmap->Insert("1111111111111111111111111", tmp, 25, 0);
  // pmem_hashmap->Insert("4444444444444444444444444", tmp, 25, 0);
  // pmem_hashmap->Insert("3333333333333333333333333", tmp, 25, 0);
  // pmem_hashmap->Insert("2222222222222222222222222", tmp, 25, 0);
  // pmem_hashmap->Insert("5555555555555555555555555", tmp, 25, 0);

  // printf("## End Insertion\n");

  printf("## Start PrintAll\n");
  pmem_hashmap->PrintAll(0);

  printf("## End PrintAll\n");


  // printf("## Start next test\n");
  // PMEMoid* first = pmem_hashmap->GetFirstOID(0);
  // TOID(struct entry) first_toid(*first);
  // struct entry* first_tmp = (struct entry *)pmemobj_direct(*first);
  // PMEMoid first_key = first_tmp->key;
  // uint8_t key_len = first_tmp->key_len;
  // std::string str_key((char *)pmemobj_direct(first_key), key_len);
  // printf("First key %d] %s\n", key_len, str_key.c_str());

  // PMEMoid* next = pmem_hashmap->GetNextOID(0, first_toid);
  // struct entry* next_tmp = (struct entry *)pmemobj_direct(*next);
  // PMEMoid next_key = next_tmp->key;
  // uint8_t key_len2 = next_tmp->key_len;
  // std::string str_key2((char *)pmemobj_direct(next_key), key_len2);
  // printf("Next key %d] %s\n", key_len2, str_key2.c_str());

  // PMEMoid* seek = pmem_hashmap->SeekOID(0, "2222222222222222222222222", 25);
  // struct entry* seek_tmp = (struct entry *)pmemobj_direct(*seek);
  // PMEMoid seek_key = seek_tmp->key;
  // uint8_t key_len3 = seek_tmp->key_len;
  // std::string str_key3((char *)pmemobj_direct(seek_key), key_len3);
  // printf("Seek key %d] %s\n", key_len3, str_key3.c_str());

  // printf("## End next test\n");

  // hm_atomic_create(hashmap_pool.get_handle(), &(root_hashmap_[0].head), nullptr);

  // TEST: Hashmap-size test
  
	printf("# End Hashmap\n");
}

} // namespace leveldb

/* Main */
int main(int argc, char** argv) {
  return leveldb::test::RunAllTests();
}
/* End Main */
