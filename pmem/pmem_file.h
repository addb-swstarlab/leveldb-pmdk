/*
 *
 */

// LEVELDB
#include "leveldb/env.h"
// #include "leveldb/status.h"
// #include "leveldb/slice.h"

// PMDK
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/mutex.hpp>

#define MAX_ARRAY_SIZE 4000000 // 4MB
#define NUM_OF_FILE 500

namespace pobj = pmem::obj;

namespace leveldb {
struct rootFile;
// Public function for PmemFile 
// Readable
//    Sequential
// ssize_t PmemRead(PmemFile* pmemfile, size_t n, char* scratch);
// Status PmemSkip(PmemFile* pmemfile,uint64_t n);
//    RandomAccess
// ssize_t PmemRead(PmemFile* pmemfile,uint64_t offset, size_t n, char* scratch);
// Writable
// ssize_t PmemAppend(PmemFile* pmemfile,const char* data, size_t n);

struct rootFile {
  pobj::persistent_ptr<char[]> contents;                   // Total 1.65GB, each 4MB
  pobj::persistent_ptr<uint32_t[]> contents_size;          // 400 Files
  pobj::persistent_ptr<uint32_t[]> current_index;    // 400 Files
};
struct rootOffset {
  pobj::persistent_ptr<uint32_t[]> fname;                  // 4000 Files
  pobj::persistent_ptr<uint32_t[]> start_index;            // 4000 Files
};
} // namespace LEVELDB