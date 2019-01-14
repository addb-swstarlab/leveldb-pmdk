// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "leveldb/env.h"

#include "port/port.h"
#include "util/testharness.h"
#include "util/env_posix_test_helper.h"

// JH
#include <iostream>

namespace leveldb {

static const int kDelayMicros = 100000;
static const int kReadOnlyFileLimit = 4;
static const int kMMapLimit = 4;

class EnvPosixTest {
 public:
  Env* env_;
  EnvPosixTest() : env_(Env::Default()) { }

  static void SetFileLimits(int read_only_file_limit, int mmap_limit) {
    EnvPosixTestHelper::SetReadOnlyFDLimit(read_only_file_limit);
    EnvPosixTestHelper::SetReadOnlyMMapLimit(mmap_limit);
  }
};

// TEST(EnvPosixTest, TestOpenOnRead) {
//   // Write some test data to a single file that will be opened |n| times.
//   std::string test_dir;
//   ASSERT_OK(env_->GetTestDirectory(&test_dir));
//   std::string test_file = test_dir + "/open_on_read.txt";

//   FILE* f = fopen(test_file.c_str(), "w");
//   ASSERT_TRUE(f != nullptr);
//   const char kFileData[] = "abcdefghijklmnopqrstuvwxyz";
//   fputs(kFileData, f);
//   fclose(f);

//   // Open test file some number above the sum of the two limits to force
//   // open-on-read behavior of POSIX Env leveldb::RandomAccessFile.
//   const int kNumFiles = kReadOnlyFileLimit + kMMapLimit + 5;
//   leveldb::RandomAccessFile* files[kNumFiles] = {0};
//   for (int i = 0; i < kNumFiles; i++) {
//     ASSERT_OK(env_->NewRandomAccessFile(test_file, &files[i]));
//   }
//   char scratch;
//   Slice read_result;
//   for (int i = 0; i < kNumFiles; i++) {
//     ASSERT_OK(files[i]->Read(i, 1, &read_result, &scratch));
//     std::cout << read_result.ToString().c_str() << "\n";
//     ASSERT_EQ(kFileData[i], read_result[0]);
//   }
//   for (int i = 0; i < kNumFiles; i++) {
//     delete files[i];
//   }
//   ASSERT_OK(env_->DeleteFile(test_file));
// }

TEST(EnvPosixTest, TestWritePmem) {
  // Write some test data to a single file that will be opened |n| times.
  std::string pmem_dir = "/home/hwan/pmem_dir";
  std::string test_file2 = pmem_dir + "/pmem.txt";

  Status s;
  // leveldb::WritableFile* file;
  // env_->NewWritableFile(test_file2, &file);
  // Slice data = Slice("abcdefghijklmnopqrstuvwxyz", 26);
  // Status s = file->Append(data);
  // file->Close();
  // delete file;
  ASSERT_OK(s);
}

TEST(EnvPosixTest, TestWriteAndAppendPmem) {
  // Write some test data to a single file that will be opened |n| times.
  std::string pmem_dir = "/home/hwan/pmem_dir";
  std::string test_file2 = pmem_dir + "/000156.ldb";

  leveldb::WritableFile* file;
  env_->NewWritableFile(test_file2, &file);
  Slice data = Slice("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefgh", 138);
  // Slice data = Slice("abcdefghijklmn", 100);

  // printf("[TEST]Append %s \n", data.data());
  // Status s = file->Append(data);
  // file->Flush();
  // s = file->Append(data);
  // file->Flush();
  // s = file->Append(data);
  // file->Flush();
  Status s;
  for (int i=0; i<25000; i++ ) {
  //   printf("[%d Steps]\n", i);
    s = file->Append(data);
    // file->Flush();
  }
  
  file->Close();
  delete file;
  ASSERT_OK(s);
}
// TEST(EnvPosixTest, TestWriteMultipleArrayPmem) {
//   // Write some test data to a single file that will be opened |n| times.
//   std::string pmem_dir = "/home/hwan/pmem_dir";
//   std::string test_file2 = pmem_dir + "/pmem2.txt";

//   leveldb::WritableFile* file;
//   env_->NewWritableFile(test_file2, &file);
//   Slice data = Slice("abcdefghijklmnopqrstuvwxyz", 10000);
//   // Slice data = Slice("abcdefghijklmn", 14000);

//   // printf("[TEST]Append %s \n", data.data());
//   // Status s = file->Append(data);
//   // file->Flush();
//   // s = file->Append(data);
//   // file->Flush();
//   // s = file->Append(data);
//   // file->Flush();
//   Status s;
//   for (int i=0; i<1; i++ ) {
//     printf("[%d Steps]\n", i);
//     s = file->Append(data);
//     file->Flush();
//   }
//   file->Close();
//   delete file;
//   ASSERT_OK(s);
// }

// TEST(EnvPosixTest, TestSequentialReadPmem) {
//   // Write some test data to a single file that will be opened |n| times.
//   std::string pmem_dir = "/home/hwan/pmem_dir";
//   std::string test_file2 = pmem_dir + "/000156.ldb";

//   leveldb::SequentialFile* file;
//   env_->NewSequentialFile(test_file2, &file);

//   static const int kBufferSize = 250;
//   char* space = new char[kBufferSize];
//   Slice data;
//   Status s = file->Read(kBufferSize, &data, space);
//   printf("End] data: %s %d\n", data.data(), data.size());
//   s = file->Read(kBufferSize, &data, space);
//   printf("Skip End] data: %s, %d\n", data.data(), data.size());
//   std::cout<< data.empty() << std::endl;
//   delete file;
//   ASSERT_OK(s);
// }

// TEST(EnvPosixTest, TestRandomReadPmem) {
//   // Write some test data to a single file that will be opened |n| times.
//   std::string pmem_dir = "/home/hwan/pmem_dir";
//   std::string test_file2 = pmem_dir + "/000156.ldb";

//   leveldb::RandomAccessFile* file;
//   env_->NewRandomAccessFile(test_file2, &file);

//   static const int kBufferSize = 4018;
//   char* space = new char[kBufferSize];
//   Slice data;
//   Status s = file->Read(5, kBufferSize, &data, space);
//   printf("End] data: %s\n", data.data());
//   s = file->Read(3400000, kBufferSize, &data, space);
//   printf("End2] data: %s\n", data.data());
//   delete file;
//   ASSERT_OK(s);
// }

}  // namespace leveldb

int main(int argc, char** argv) {
  // All tests currently run with the same read-only file limits.
  leveldb::EnvPosixTest::SetFileLimits(leveldb::kReadOnlyFileLimit,
                                       leveldb::kMMapLimit);
  return leveldb::test::RunAllTests();
}
