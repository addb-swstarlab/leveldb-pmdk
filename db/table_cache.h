// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Thread-safe (provides internal synchronization)

#ifndef STORAGE_LEVELDB_DB_TABLE_CACHE_H_
#define STORAGE_LEVELDB_DB_TABLE_CACHE_H_

#include <string>
#include <stdint.h>
#include "db/dbformat.h"
#include "leveldb/cache.h"
#include "leveldb/table.h"
#include "port/port.h"

// JH
// #include <iostream>
// #include <fstream>
// #include <libpmemobj++/make_persistent.hpp>
// #include <libpmemobj++/make_persistent_atomic.hpp>
// #include <libpmemobj++/p.hpp>
// #include <libpmemobj++/persistent_ptr.hpp>
// #include <libpmemobj++/pool.hpp>
// #define POOLID "pool"

// namespace pobj = pmem::obj;

namespace leveldb {

class Env;

// Customized by JH
/*
 * File은 우리가 생각하는 파일시스템의 파일을 객체화. 곧 실제 파일을 이야기할 수 있으므로 가시적
 * Table은 levelDB에서 구조화한 SSTable을 의미함. 즉, 추상적인 형체
 * 그렇다면 TableAndFile은 특정 Table이 위치한 File객체를 묶어서 표현한 것으로 보임
*/
struct TableAndFile {
  RandomAccessFile* file;
  Table* table;
};
class Sample {
 public:
  Sample(uint64_t input);
  ~Sample();
  void insert(uint64_t input);
  void printContent();
  int printCount();

 private:
  uint64_t value[200];
  int count;

};


class TableCache {
 public:
  TableCache(const std::string& dbname, const Options& options, int entries);
  ~TableCache();

  // Return an iterator for the specified file number (the corresponding
  // file length must be exactly "file_size" bytes).  If "tableptr" is
  // non-null, also sets "*tableptr" to point to the Table object
  // underlying the returned iterator, or to nullptr if no Table object
  // underlies the returned iterator.  The returned "*tableptr" object is owned
  // by the cache and should not be deleted, and is valid for as long as the
  // returned iterator is live.
  Iterator* NewIterator(const ReadOptions& options,
                        uint64_t file_number,
                        uint64_t file_size,
                        Table** tableptr = nullptr);

  // If a seek to internal key "k" in specified file finds an entry,
  // call (*handle_result)(arg, found_key, found_value).
  Status Get(const ReadOptions& options,
             uint64_t file_number,
             uint64_t file_size,
             const Slice& k,
             void* arg,
             void (*handle_result)(void*, const Slice&, const Slice&));

  // Evict any entry for the specified file number
  void Evict(uint64_t file_number);
/*
  struct root {
	  // pobj::persistent_ptr<TableAndFile> taf;
	  pobj::persistent_ptr<Sample> sample;
  };

  // JH
  pobj::pool<root> pop;
	// pobj::persistent_ptr<root> pool;
  pobj::persistent_ptr<root> GetPersistptr();

*/
 private:
  Env* const env_;
  const std::string dbname_;
  const Options& options_;
  Cache* cache_;
/*
  // JH
  // pobj::pool<root> pop;
	pobj::persistent_ptr<root> pool;
*/
  

  Status FindTable(uint64_t file_number, uint64_t file_size, Cache::Handle**);
};


/* root structure  */
/****************************
 *This root structure contains all the connections the pool and persistent
 *pointer to the persistent objects. Using this root structure component to
 *access the pool and print out the message. 
 ******************************/
// struct root {
// 	// pobj::persistent_ptr<TableAndFile> taf;
// 	pobj::persistent_ptr<Sample> sample;
// };
  // JH
// pobj::pool<root> pop;
// pobj::persistent_ptr<root> pool;
/*
inline bool
file_exists (const std::string &name)
{
	std::ifstream f (name.c_str());
	return f.good ();
}*/

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_TABLE_CACHE_H_
