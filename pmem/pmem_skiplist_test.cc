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
#include "pmem/pmem_skiplist.h"

#define NUM_SKIPLISTS 3


using namespace std;

namespace leveldb {


class PmemSkiplistTest { };

inline bool
file_exists (const std::string &name)
{
	std::ifstream f (name.c_str ());
	return f.good ();
}

TEST (PmemSkiplistTest, Skiplist_manager) {
	cout << "# Start Skiplist_manager" << endl;

  printf("## Make persistent PmemSkiplist class\n");
  PmemSkiplist **pmem_skiplists = new PmemSkiplist*[NUM_OF_SKIPLIST_MANAGER];
  // Create
  // for (int i=0; i < NUM_SKIPLISTS; i++) {
    printf("Create 0\n");
    pmem_skiplists[0] = new PmemSkiplist(SKIPLIST_MANAGER_PATH_0);
    printf("Create 1\n");
    pmem_skiplists[1] = new PmemSkiplist(SKIPLIST_MANAGER_PATH_1);
    printf("Create 2\n");
    pmem_skiplists[2] = new PmemSkiplist(SKIPLIST_MANAGER_PATH_2);
    printf("Create 3\n");
    pmem_skiplists[3] = new PmemSkiplist(SKIPLIST_MANAGER_PATH_3);
    printf("Create 4\n");
    pmem_skiplists[4] = new PmemSkiplist(SKIPLIST_MANAGER_PATH_4);
    printf("Create 5\n");
    pmem_skiplists[5] = new PmemSkiplist(SKIPLIST_MANAGER_PATH_5);
    printf("Create 6\n");
    pmem_skiplists[6] = new PmemSkiplist(SKIPLIST_MANAGER_PATH_6);
    printf("Create 7\n");
    pmem_skiplists[7] = new PmemSkiplist(SKIPLIST_MANAGER_PATH_7);
    printf("Create 8\n");
    pmem_skiplists[8] = new PmemSkiplist(SKIPLIST_MANAGER_PATH_8);
    printf("Create 9\n");
    pmem_skiplists[9] = new PmemSkiplist(SKIPLIST_MANAGER_PATH_9);
    printf("Create end\n");
  // }

  // Bulk insert
  // for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
  //   for (int j=0; j<5; j++) {
  //     char key[] = "key-";
  //     stringstream ss_key, ss_value;
  //     ss_key << key << i << "-" << j;
  //     char value[] = "value-";
  //     // make max 100bytes
  //     ss_value << value << i << "-" << j 
  //     << ".......................................................................................";
  //     // << "..............................................";
  //     // printf("'%s'-'%s'\n", (char *)ss_key.str().c_str(), (char *)ss_value.str().c_str());
  //     pmem_skiplists[0]->Insert((char *)ss_key.str().c_str(), 
  //               (char *)ss_value.str().c_str(), 
  //               ss_key.str().size(), ss_value.str().size(), i);
  //     // skiplist_map_create(pbac_skiplists[0]->GetPool(), &(pbac_skiplists[0]->skiplists_[i]), i, nullptr);
  //     // skiplist_map_insert(pbac_skiplists[0]->GetPool(), pbac_skiplists[0]->skiplists_[i], (char *)ss_key.str().c_str(),
  //     //    (char *)ss_value.str().c_str(), ss_key.str().size(), ss_value.str().size(), i);
  //     pmem_skiplists[1]->Insert((char *)ss_key.str().c_str(), 
  //               (char *)ss_value.str().c_str(), 
  //               ss_key.str().size(), ss_value.str().size(), i);
  //     // skiplist_map_insert(pbac_skiplists[1]->GetPool(), pbac_skiplists[1]->skiplists_[i], (char *)ss_key.str().c_str(),
  //     //    (char *)ss_value.str().c_str(), ss_key.str().size(), ss_value.str().size(), i);

  //   }

  //   printf("Insert %d] End\n", i);
  // }

  // Print all
  // printf("Skip 0\n");
  // for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
  //   printf("<<%d>>\n", i);
  //   pmem_skiplists[0]->PrintAll(i);
  // }
  // printf("Skip 3\n");
  // for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
  //   printf("<<%d>>\n", i);
  //   pmem_skiplists[3]->PrintAll(i);
  // }
  // printf("Skip 5\n");
  // for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
  //   printf("<<%d>>\n", i);
  //   pmem_skiplists[5]->PrintAll(i);
  // }
  printf("Skip 6\n");
  for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
    printf("<<%d>>\n", i);
    pmem_skiplists[6]->PrintAll(i);
  }
  printf("Skip 7\n");
  for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
    printf("<<%d>>\n", i);
    pmem_skiplists[7]->PrintAll(i);
  }

  // for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
  //   printf("<< %d >>\n", i);
  //   printf("skip 0]\n");
  //   pmem_skiplists[0]->PrintAll(i);
  //   printf("skip 1]\n");
  //   pmem_skiplists[1]->PrintAll(i);
  //   printf("skip 2]\n");
  //   pmem_skiplists[2]->PrintAll(i);
  //   printf("skip 3]\n");
  //   pmem_skiplists[3]->PrintAll(i);
  //   printf("skip 4]\n");
  //   pmem_skiplists[4]->PrintAll(i);
  //   printf("skip 5]\n");
  //   pmem_skiplists[5]->PrintAll(i);
  //   printf("skip 6]\n");
  //   pmem_skiplists[6]->PrintAll(i);
  //   printf("skip 7]\n");
  //   pmem_skiplists[7]->PrintAll(i);
  //   printf("skip 8]\n");
  //   pmem_skiplists[8]->PrintAll(i);
  //   printf("skip 9]\n");
  //   pmem_skiplists[9]->PrintAll(i);
  // }
	/* Close persistent pool */
	printf("# End Skiplist_map\n");
}

} // namespace leveldb

/* Main */
int main(int argc, char** argv) {
  return leveldb::test::RunAllTests();
}
/* End Main */
