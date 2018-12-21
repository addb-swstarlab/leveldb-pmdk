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
// #include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/mutex.hpp>

// #include <string>

// #define MAX_FILESIZE 100000 // 64MB

namespace pobj = pmem::obj;

namespace leveldb {
struct rootFile;
class PmemFile {
 public:
  PmemFile();
  PmemFile(pobj::pool<rootFile> pool);
  ~PmemFile();

  // Readable
  //    Sequential
  Status Read(size_t n, Slice* result, char* scratch);
  Status Skip(uint64_t n);
  //    RandomAccess
  Status Read(uint64_t offset, size_t n, Slice* result, char* scratch);

  // Writable
  Status Append(const Slice& data);
  // Status Append(pobj::persistent_ptr<>);

  int getContentsSize();
  
 private: 
  pobj::pool<rootFile> pool;
  pobj::persistent_ptr<char[]> contents;
  // pobj::p<char[]> filter;
  // pobj::p<char[]> metaindex;
  // pobj::p<char[]> index;
  pobj::p<int> contents_size;
  // pobj::p<int> filter_size;
  // pobj::p<int> metaindex_size;
  // pobj::p<int> index_size;
  pobj::mutex mutex;
};

struct rootFile{
  pobj::persistent_ptr<PmemFile> file;
};
} // namespace LEVELDB