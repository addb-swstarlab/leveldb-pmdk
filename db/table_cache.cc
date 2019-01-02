// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "db/table_cache.h"

#include "db/filename.h"
#include "leveldb/env.h"
#include "leveldb/table.h"
#include "util/coding.h"

#include <iostream>

// namespace pobj = pmem::obj;

namespace leveldb {
/*
 * File은 우리가 생각하는 파일시스템의 파일을 객체화. 곧 실제 파일을 이야기할 수 있으므로 가시적
 * Table은 levelDB에서 구조화한 SSTable을 의미함. 즉, 추상적인 형체
 * 그렇다면 TableAndFile은 특정 Table이 위치한 File객체를 묶어서 표현한 것으로 보임
*/
// struct TableAndFile {
//   RandomAccessFile* file;
//   Table* table;
// };

static void DeleteEntry(const Slice& key, void* value) {
  TableAndFile* tf = reinterpret_cast<TableAndFile*>(value);
  delete tf->table;
  delete tf->file;
  delete tf;
}

static void UnrefEntry(void* arg1, void* arg2) {
  Cache* cache = reinterpret_cast<Cache*>(arg1);
  Cache::Handle* h = reinterpret_cast<Cache::Handle*>(arg2);
  cache->Release(h);
}

TableCache::TableCache(const std::string& dbname,
                       const Options& options,
                       int entries)
    : env_(options.env),
      dbname_(dbname),
      options_(options),
      cache_(NewLRUCache(entries)) {
}

TableCache::~TableCache() {
  delete cache_;
}

// table_cache -> get
Status TableCache::FindTable(uint64_t file_number, uint64_t file_size,
                             Cache::Handle** handle) {
  Status s;
  char buf[sizeof(file_number)];
  EncodeFixed64(buf, file_number);
  Slice key(buf, sizeof(buf));
  // table cache에서 해당 key에 대한 매핑이 있으면 Cache handle을 반환받음
  *handle = cache_->Lookup(key);
  // table cache에 해당 key에 대한 매핑이 없으면 새로 만들어서 insert해줌
  if (*handle == nullptr) {
    std::string fname = TableFileName(dbname_, file_number);
    RandomAccessFile* file = nullptr;
    Table* table = nullptr;
    s = env_->NewRandomAccessFile(fname, &file);
    if (!s.ok()) {
      std::string old_fname = SSTTableFileName(dbname_, file_number);
      if (env_->NewRandomAccessFile(old_fname, &file).ok()) {
        s = Status::OK();
      }
    }
    if (s.ok()) {
      s = Table::Open(options_, file, file_size, &table);
    }

    if (!s.ok()) {
      assert(table == nullptr);
      delete file;
      // We do not cache error results so that if the error is transient,
      // or somebody repairs the file, we recover automatically.
    } else {
      TableAndFile* tf = new TableAndFile;
      tf->file = file;
      tf->table = table;
      std::cout <<"DEBUG 1]\n";
      *handle = cache_->Insert(key, tf, 1, &DeleteEntry);
      std::cout <<"DEBUG 2]\n";

      /*
      std::string file = "/home/hwan/pmem_dir/table_cache";
      // JH
      TableCache* table_cache = reinterpret_cast<TableCache*>(cache_);
      // Get the root object
      if (file_exists(file)) {
        table_cache->pop = pobj::pool<root>::open (file, POOLID);
        table_cache->pool = table_cache->pop.get_root();
        table_cache->pool->sample->insert(file_number);
        // std::cout << table_cache->pool << std::endl;
        // table_cache->pool->sample->printContent();
      } else {
      	table_cache->pop = pobj::pool<root>::create (file, POOLID,
									 PMEMOBJ_MIN_POOL, S_IRUSR | S_IWUSR);
        table_cache->pool = table_cache->pop.get_root();
      	// Store the input into persistent memory
      	pobj::make_persistent_atomic<Sample> (table_cache->pop, table_cache->pool->sample, file_number);
        // table_cache->pool->sample->printContent();
      }
      table_cache->pop.close();
      */
    }
  }
  return s;
}

Iterator* TableCache::NewIterator(const ReadOptions& options,
                                  uint64_t file_number,
                                  uint64_t file_size,
                                  Table** tableptr) {
  if (tableptr != nullptr) {
    *tableptr = nullptr;
  }

  Cache::Handle* handle = nullptr;
  Status s = FindTable(file_number, file_size, &handle);
  if (!s.ok()) {
    return NewErrorIterator(s);
  }

  Table* table = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->table;
  std::cout<< "create table newIterator\n";
  Iterator* result = table->NewIterator(options);
  result->RegisterCleanup(&UnrefEntry, cache_, handle);
  if (tableptr != nullptr) {
    *tableptr = table;
  }
  return result;
}

// DBImpl -> VersionSet -> TableCache
Status TableCache::Get(const ReadOptions& options,
                       uint64_t file_number,
                       uint64_t file_size,
                       const Slice& k,
                       void* arg,
                       void (*saver)(void*, const Slice&, const Slice&)) {
  Cache::Handle* handle = nullptr;
  Status s = FindTable(file_number, file_size, &handle); // table cache로 부터 handle을 받음
  if (s.ok()) {
    Table* t = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->table; //
    s = t->InternalGet(options, k, arg, saver);
    cache_->Release(handle);
  }
  return s;
}

void TableCache::Evict(uint64_t file_number) {
  char buf[sizeof(file_number)];
  EncodeFixed64(buf, file_number);
  cache_->Erase(Slice(buf, sizeof(buf)));
}

// JH
/*
pobj::persistent_ptr<TableCache::root> TableCache::GetPersistptr() {
  return pool;
}
Sample::Sample (uint64_t input) {
  value[100] = {input};
   count = 1;
}
void Sample::insert(uint64_t input) {
  value[count++] = input;
}
void Sample::printContent() {
  for (int i=0; i < count; i++) {
    std::cout << "[" << i << "]" << value[i] << std::endl;
  }
}
int Sample::printCount() {
  return count;
}
*/
}  // namespace leveldb
