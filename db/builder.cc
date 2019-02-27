// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "db/builder.h"

#include "db/filename.h"
#include "db/dbformat.h"
#include "db/table_cache.h"
#include "db/version_edit.h"
#include "leveldb/db.h"
#include "leveldb/env.h"
#include "leveldb/iterator.h"

// temp
#include <chrono>
#include <iostream>

namespace leveldb {

/* TODO: Write file based on pmem */
// /*
Status BuildTable(const std::string& dbname,
                  Env* env,
                  const Options& options,
                  TableCache* table_cache,
                  Iterator* iter,
                  FileMetaData* meta) {
  PmemSkiplist* pmem_skiplist = options.pmem_skiplist;
  Status s;
  meta->file_size = 0;
  iter->SeekToFirst();

  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
  std::string fname = TableFileName(dbname, meta->number);
  if (iter->Valid()) {
    WritableFile* file;
    s = env->NewWritableFile(fname, &file);
    if (!s.ok()) {
      return s;
    }

    TableBuilder* builder = new TableBuilder(options, file);
    meta->smallest.DecodeFrom(iter->key());

    uint64_t file_number;
    FileType type;
    if (ParseFileName(fname.substr(fname.rfind("/")+1, fname.size()), &file_number, &type) &&
          type != kDBLockFile) {
        for (; iter->Valid(); iter->Next()) {
          Slice key = iter->key();
          meta->largest.DecodeFrom(key);
          builder->AddToPmem(pmem_skiplist, file_number, key, iter->value());
          // printf("'%s'-'%s', '%d' '%d'\n", key.data(), iter->value().data(), key.size(), iter->value().size());
        }
    } else {
      printf("[ERROR] Invalid filename '%s' '%d'\n", fname.c_str(), file_number);
      s = Status::InvalidArgument(Slice());
    }
    meta->file_size = builder->FileSize();
    assert(meta->file_size > 0);

    // [TableBuilder] Add -> Finish.
    // [file] Sync -> Close
    // printf("[%s]i: %d\n", fname.c_str(), i);
  }


    std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();
    std::cout << "result = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<std::endl;
  return s;
}
// */
/* Original version */
/*
Status BuildTable(const std::string& dbname,
                  Env* env,
                  const Options& options,
                  TableCache* table_cache,
                  Iterator* iter,
                  FileMetaData* meta) {
  Status s;
  meta->file_size = 0;
  iter->SeekToFirst();

  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
  std::string fname = TableFileName(dbname, meta->number);
  if (iter->Valid()) {
    WritableFile* file;
    s = env->NewWritableFile(fname, &file);
    if (!s.ok()) {
      return s;
    }

    TableBuilder* builder = new TableBuilder(options, file);
    meta->smallest.DecodeFrom(iter->key());

    // int i = 0;
    for (; iter->Valid(); iter->Next()) {
      Slice key = iter->key();
      meta->largest.DecodeFrom(key);
      builder->Add(key, iter->value());
      // 24, 100
      // printf("'%s'-'%s', '%d' '%d'\n", key.data(), iter->value().data(), key.size(), iter->value().size());
      // i++;
    }
        // printf("[%s]i: %d\n", fname.c_str(), i);

    // Finish and check for builder errors
    s = builder->Finish();
    if (s.ok()) {
      meta->file_size = builder->FileSize();
      assert(meta->file_size > 0);
    }
    delete builder;

    // Finish and check for file errors
    if (s.ok()) {
      s = file->Sync();
    }
    if (s.ok()) {
      s = file->Close();
    }
    delete file;
    file = nullptr;

    if (s.ok()) {
      // Verify that the table is usable
      Iterator* it = table_cache->NewIterator(ReadOptions(),
                                              meta->number,
                                              meta->file_size);
      s = it->status();
      delete it;
    }
  }

  // Check for input iterator errors
  if (!iter->status().ok()) {
    s = iter->status();
  }

  if (s.ok() && meta->file_size > 0) {
    // Keep it
  } else {
    env->DeleteFile(fname);
  }

    std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();
    std::cout << "result = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<std::endl;
  return s;
}
*/
}  // namespace leveldb

