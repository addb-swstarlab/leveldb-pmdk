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
#include <errno.h>

namespace leveldb {

class PmemMapTest { };

/* Functions */

/* file_exists function  */
/****************************
 * This checks to see if the filename exists.  A boolean value (true or false)
 * will return to the calling function.
 *
 *****************************/
inline bool
file_exists (const std::string &name)
{
	std::ifstream f (name.c_str ());
	return f.good ();
}

/* End Functions */

/****************************
 * This function writes the "Hello..." string to persistent-memory.
 *****************************/
// void write_hello_string (char *input, char *path)
// {
// 	pobj::pool<root> pop;
// 	pobj::persistent_ptr<root> pool;
	
// 	/* Create pool in persistent memory */
// 	// Get the root object
// 	pop = pobj::pool<root>::create (path, LAYOUT,
// 									 PMEMOBJ_MIN_POOL, S_IRUSR | S_IWUSR);
// 	// Get pool object
// 	pool = pop.get_root ();
	
// 	// Store the input into persistent memory
// 	pobj::make_persistent_atomic<Hello> (pop, pool->hello, input);
	
// 	// Write to the console
// 	cout << endl << "\nWrite the (" << pool->hello->get_hello_msg()
// 	 << ") string to persistent-memory." << endl;	
			
// 	/* Cleanup */
// 	/* Close persistent pool */
// 	pop.close ();	
// 	return;
// }

/****************************
 * This function reads the "Hello..." string from persistent-memory.
 *****************************/
// std::string read_hello_string(char *path)
// {
// 	pobj::pool<root> pop;
// 	pobj::persistent_ptr<root> pool;

// 	/* Open the pool in persistent memory */
// 	pop = pobj::pool<root>::open (path, LAYOUT);
// 	pool = pop.get_root ();

// 	std::string result = pool->hello->get_hello_msg();
// 	// Write to the console
// 	cout << endl    << "\nRead the ("	<< result
// 		<< ") string from persistent-memory." << endl;		
	
// 	/* Cleanup */
// 	/* Close persistent pool */
// 	pop.close ();	

// 	return result;
// }


/* Main */
TEST (PmemMapTest, HelloWorld) {
	pobj::pool<root> pool1;
	pobj::persistent_ptr<root> ptr;
	
	cout << "# Start " << endl;
	// Prepare the input to store into persistent memory

	// mutex + string test
	if (!file_exists(PMAP_PATH)) {
		pool1 = pobj::pool<root>::create (PMAP_PATH, POOL1,
									 PMAP_SIZE, S_IRUSR | S_IWUSR);
		// Get pool object
		ptr = pool1.get_root ();		
		pobj::transaction::exec_tx( pool1, [&] {
			ptr->pmap_ptr = pobj::make_persistent<PmemMap>();
		});
	} 
	else {
		pool1 = pobj::pool<root>::open (PMAP_PATH, POOL1);
		ptr = pool1.get_root ();		
		pobj::transaction::exec_tx( pool1, [&] {
			ptr->pmap_ptr = pobj::make_persistent<PmemMap>();
		});
	}

  
	cout << "# Center " << endl;
	// ASSERT_EQ(0 , strcmp("mutex1", "mutex1"));

	/* Cleanup */
	cout << "# End " << endl;
  pobj::transaction::exec_tx( pool1, [&] {
    pobj::delete_persistent<PmemMap>(ptr->pmap_ptr);
	});
	/* Close persistent pool */
	pool1.close ();	
}

} // namespace leveldb

/* Main */
int main(int argc, char** argv) {
  return leveldb::test::RunAllTests();
}
/* End Main */
