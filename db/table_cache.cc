// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "db/table_cache.h"

#include "db/filename.h"
#include "leveldb/env.h"
#include "leveldb/table.h"
#include "util/coding.h"

namespace leveldb {
/*
 * File은 우리가 생각하는 파일시스템의 파일을 객체화. 곧 실제 파일을 이야기할 수 있으므로 가시적
 * Table은 levelDB에서 구조화한 SSTable을 의미함. 즉, 추상적인 형체
 * 그렇다면 TableAndFile은 특정 Table이 위치한 File객체를 묶어서 표현한 것으로 보임
*/
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

// table_cache -> get
Status TableCache::FindTable(uint64_t file_number, uint64_t file_size,
                             Cache::Handle** handle) {
  Status s;
  char buf[sizeof(file_number)];
  EncodeFixed64(buf, file_number); // memcpy[file_number -> buf]
  Slice key(buf, sizeof(buf));
  // 1) table cache에서 해당 file number에 대한 매핑이 있으면 Cache handle을 반환받음
  *handle = cache_->Lookup(key);
  // 2) table cache에 해당 file number에 대한 매핑이 없으면 새로 만들어서 insert해줌
  // Log(options_.info_log, "Before handle nullptr");
  if (*handle == nullptr) {
    Log(options_.info_log, "[WARN] %s(%lld) cannot find key in table_cache..", key.ToString().c_str() , file_number);
    std::string fname = TableFileName(dbname_, file_number);
    RandomAccessFile* file = nullptr;
    Table* table = nullptr;
    // 파일명(ldb)을 바탕으로 열을 File 객체 생성
    s = env_->NewRandomAccessFile(fname, &file);
    // 파일객체 생성에 대한 실패 (why??)
    if (!s.ok()) {
      // Log(options_.info_log, "handle not ok 1 ");
      std::string old_fname = SSTTableFileName(dbname_, file_number);
      if (env_->NewRandomAccessFile(old_fname, &file).ok()) {
        s = Status::OK();
      }
    }
    // 객체는 생성 성공. 그래서 열어본 것을 table 객체에 저장하고 싶음
    if (s.ok()) {
      // Log(options_.info_log, "handle ok 1 ");
      s = Table::Open(options_, file, file_size, &table);
    }

    // 읽을라고 열었더니 실패. PosixRandomAccessFile::Read
    if (!s.ok()) {
      Log(options_.info_log, "handle not ok 2");
      assert(table == nullptr);
      delete file;
      // We do not cache error results so that if the error is transient,
      // or somebody repairs the file, we recover automatically.
    } else {
      Log(options_.info_log, "handle ok 2 ");
      TableAndFile* tf = new TableAndFile;
      tf->file = file;
      tf->table = table;
      *handle = cache_->Insert(key, tf, 1, &DeleteEntry);
    }
  }
  // JH for DEBUG
  else {
    Log(options_.info_log, "[DEBUG] Found %s(%lld) in table_cache!!", key.ToString().c_str(), file_number);
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

  // 1) Find table from table_cache ==> store to handle 
  Cache::Handle* handle = nullptr;
  Status s = FindTable(file_number, file_size, &handle); // *handle = cache_->Insert(key, tf, 1, &DeleteEntry);
  if (!s.ok()) {
    Log(options_.info_log, "[Find Table] Error");
    return NewErrorIterator(s); // Empty Iter + current status
  }
  // JH
  // 2) cache handle로부터 table을 get. table로부터 iterator 생성
  Table* table = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->table;
  Iterator* result = table->NewIterator(options); // NewTwoLevelIterator
  result->RegisterCleanup(&UnrefEntry, cache_, handle); // cleanup에 대한 job을 node에 등록하는듯. cleanup은 cache로부터 handle release
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
// When FinishCompactionOutputFile is called, 
// Insert output into table cache
Status TableCache::FinishCompactionOutputCache() {

  return Status::OK();
}

}  // namespace leveldb
