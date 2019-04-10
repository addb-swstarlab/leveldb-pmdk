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
#include "pmem/pmem_buffer.h"


using namespace std;

namespace leveldb {


class PmemBufferTest { };

inline bool
file_exists (const std::string &name)
{
	std::ifstream f (name.c_str ());
	return f.good ();
}

TEST (PmemBufferTest, Buffer) {
	cout << "# Start Pmem-Buffer" << endl;

  PmemBuffer **pmem_buffer = new PmemBuffer*[NUM_OF_BUFFER];
  // Create
    printf("Create 0\n");
    pmem_buffer[0] = new PmemBuffer(BUFFER_PATH_0);
    printf("Create 1\n");
    pmem_buffer[1] = new PmemBuffer(BUFFER_PATH_1);
    printf("Create 2\n");
    pmem_buffer[2] = new PmemBuffer(BUFFER_PATH_2);
    printf("Create 3\n");
    pmem_buffer[3] = new PmemBuffer(BUFFER_PATH_3);
    printf("Create 4\n");
    pmem_buffer[4] = new PmemBuffer(BUFFER_PATH_4);
    printf("Create 5\n");
    pmem_buffer[5] = new PmemBuffer(BUFFER_PATH_5);
    printf("Create 6\n");
    pmem_buffer[6] = new PmemBuffer(BUFFER_PATH_6);
    printf("Create 7\n");
    pmem_buffer[7] = new PmemBuffer(BUFFER_PATH_7);
    printf("Create 8\n");
    pmem_buffer[8] = new PmemBuffer(BUFFER_PATH_8);
    printf("Create 9\n");
    pmem_buffer[9] = new PmemBuffer(BUFFER_PATH_9);
    printf("Create end\n");

  pmem_buffer[0]->ClearAll();

  // std::string buffer;
  

  // for (int i=0; i<1; i++) {
  //   for (int j=0; j<10; j++) {
  //     char key[] = "key-";
  //     stringstream ss_key, ss_value;
  //     ss_key << key << i << "-" << j;
  //     char value[] = "value-";
  //     ss_value << value << i << "-" << j 
  //     << ".......................................................................................";
  //     EncodeToBuffer(&buffer, Slice(ss_key.str()), Slice(ss_value.str()));
  //     // pmem_skiplists[1]->Insert((char *)ss_key.str().c_str(), 
  //     //           (char *)ss_value.str().c_str(), 
  //     // printf("key_len:%d, value_len:%d\n",ss_key.str().size(), ss_value.str().size());

  //   }
  // }
  // AddToPmemBuffer(pmem_buffer[0], &buffer, 0);
  // printf("Complete addtion to pmem-buffer\n");


  // // std::string res_value;
  // // Get(pmem_buffer[0], key, &res_value);
  // // printf("Get %d ]'%s'\n",res_value.size(), res_value.c_str());

  // // GetAndPrintAll(pmem_buffer[0], 0);
  // Slice res;
  // pmem_buffer[0]->RandomRead(0, 0, EACH_CONTENT_SIZE, &res);
  
  // uint32_t skiped_length = SkipNEntriesAndGetOffset(res.data(), 0, 10);
  // uint32_t decoded_length = PrintKVAndReturnLength(const_cast<char *>(res.data())+skiped_length);

  printf("Complete GetAndPrintAll\n");

  // Bulk insert
  // for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
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
  // printf("Skip 6\n");
  // for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
  //   printf("<<%d>>\n", i);
  //   pmem_skiplists[6]->PrintAll(i);
  // }
  // printf("Skip 7\n");
  // for (int i=0; i<SKIPLIST_MANAGER_LIST_SIZE; i++) {
  //   printf("<<%d>>\n", i);
  //   pmem_skiplists[7]->PrintAll(i);
  // }

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
	printf("# End Buffer\n");
}

} // namespace leveldb

/* Main */
int main(int argc, char** argv) {
  return leveldb::test::RunAllTests();
}
/* End Main */
