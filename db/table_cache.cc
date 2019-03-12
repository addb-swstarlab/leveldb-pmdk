// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "db/table_cache.h"

#include "db/filename.h"
#include "leveldb/env.h"
#include "leveldb/table.h"
#include "util/coding.h"

// JH
#include <chrono>
#include <iostream>

namespace leveldb {

struct TableAndFile {
  RandomAccessFile* file;
  Table* table;
};

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

Status TableCache::FindTable(uint64_t file_number, uint64_t file_size,
                             Cache::Handle** handle) {
  Status s;
  char buf[sizeof(file_number)];
  EncodeFixed64(buf, file_number);
  Slice key(buf, sizeof(buf));
  // *handle = cache_->Lookup(key);
  // if (*handle == nullptr) {
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
      *handle = cache_->Insert(key, tf, 1, &DeleteEntry);
    }
  // }
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
  Iterator* result = table->NewIterator(options);
  result->RegisterCleanup(&UnrefEntry, cache_, handle);
  if (tableptr != nullptr) {
    *tableptr = table;
  }
  return result;
}
// JH
/* TODO: Compaction based on pmem */
Iterator* TableCache::NewIteratorFromPmem(const ReadOptions& options,
                                  uint64_t file_number,
                                  uint64_t file_size,
                                  Table** tableptr) {
                                    
  PmemSkiplist* pmem_skiplist;
  switch (file_number %10) {
    case 0: pmem_skiplist = options_.pmem_skiplist[0]; break;
    case 1: pmem_skiplist = options_.pmem_skiplist[1]; break;
    case 2: pmem_skiplist = options_.pmem_skiplist[2]; break;
    case 3: pmem_skiplist = options_.pmem_skiplist[3]; break;
    case 4: pmem_skiplist = options_.pmem_skiplist[4]; break;
    case 5: pmem_skiplist = options_.pmem_skiplist[5]; break;
    case 6: pmem_skiplist = options_.pmem_skiplist[6]; break;
    case 7: pmem_skiplist = options_.pmem_skiplist[7]; break;
    case 8: pmem_skiplist = options_.pmem_skiplist[8]; break;
    case 9: pmem_skiplist = options_.pmem_skiplist[9]; break;
  }
  if (tableptr != nullptr) {
    *tableptr = nullptr;
  }
  
  // Cache::Handle* handle = nullptr;
  // Status s = FindTable(file_number, file_size, &handle);
  // if (!s.ok()) {
  //   return NewErrorIterator(s);
  // }

  // printf("Get 1\n");
  // FIXME: Checks cache logic
  // Iterator* result = new PmemIterator(file_number/NUM_OF_SKIPLIST_MANAGER, pmem_skiplist);
  Iterator* result = new PmemIterator(file_number, pmem_skiplist);
  result->SeekToFirst();
  // printf("Get 2\n");
  // Table* table = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->table;
  // Iterator* result = table->NewIteratorFromPmem(options_, options);
  // result->RegisterCleanup(&UnrefEntry, cache_, handle);
  // if (tableptr != nullptr) {
  //   *tableptr = table;
  // }
  return result;
}
Status TableCache::Get(const ReadOptions& options,
                       uint64_t file_number,
                       uint64_t file_size,
                       const Slice& k,
                       void* arg,
                       void (*saver)(void*, const Slice&, const Slice&)) {
  Cache::Handle* handle = nullptr;
  Status s = FindTable(file_number, file_size, &handle);
  if (s.ok()) {
    Table* t = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->table;
    s = t->InternalGet(options, k, arg, saver);
    cache_->Release(handle);
  }
  return s;
}
// JH
/* SOLVE: Get based on pmem */
Status TableCache::GetFromPmem(const Options& options,
                   uint64_t file_number,
                   const Slice& k,
                   void* arg,
                   void (*saver)(void*, const Slice&, const Slice&)) {
  Status s;
  // PmemSkiplist *pmem_skiplist = options.pmem_skiplist;
  PmemIterator* pmem_iterator;
  switch (file_number %10) {
    case 0: pmem_iterator = options.pmem_internal_iterator[0]; break;
    case 1: pmem_iterator = options.pmem_internal_iterator[1]; break;
    case 2: pmem_iterator = options.pmem_internal_iterator[2]; break;
    case 3: pmem_iterator = options.pmem_internal_iterator[3]; break;
    case 4: pmem_iterator = options.pmem_internal_iterator[4]; break;
    case 5: pmem_iterator = options.pmem_internal_iterator[5]; break;
    case 6: pmem_iterator = options.pmem_internal_iterator[6]; break;
    case 7: pmem_iterator = options.pmem_internal_iterator[7]; break;
    case 8: pmem_iterator = options.pmem_internal_iterator[8]; break;
    case 9: pmem_iterator = options.pmem_internal_iterator[9]; break;
  }
  // pmem_iterator->SetIndex(file_number);
  // printf("3 %d\n", file_number);
  // PmemIterator *pmem_iterator = new PmemIterator(file_number, pmem_skiplist);
  // printf("1\n");
	// std::chrono::steady_clock::time_point begin, end;
	// begin = std::chrono::steady_clock::now();
  // pmem_iterator->SetIndexAndSeek(file_number/NUM_OF_SKIPLIST_MANAGER, k);
  pmem_iterator->SetIndexAndSeek(file_number, k);

	// end= std::chrono::steady_clock::now();
	// std::cout << "SetIndexAndSeek = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<"\n";
  // printf("2\n");
  // pmem_iterator->Seek(k);
  // Slice key(pmem_iterator->key());
  // Slice value(pmem_iterator->value());
  // (*saver)(arg, key, value);
  (*saver)(arg, pmem_iterator->key(), pmem_iterator->value());
  // printf("Get 2\n");
  // printf("key1:'%s'\n", pmem_iterator->key());
  // printf("value1:'%s'\n", pmem_iterator->value());

  // TEST:
  // pmem_iterator->SeekToLast();
  // printf("key2:'%s'\n", pmem_iterator->key());
  // printf("value2:'%s'\n", pmem_iterator->value());

  return s;
}


void TableCache::Evict(uint64_t file_number) {
  char buf[sizeof(file_number)];
  EncodeFixed64(buf, file_number);
  cache_->Erase(Slice(buf, sizeof(buf)));
}

}  // namespace leveldb
