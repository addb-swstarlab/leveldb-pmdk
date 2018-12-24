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
// #define BUFFER_SIZE 4294967295

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
  ssize_t Read(size_t n, char* scratch);
  Status Skip(uint64_t n);
  //    RandomAccess
  ssize_t Read(uint64_t offset, size_t n, char* scratch);
  // Writable
  Status Append(const Slice& data);

  int getContentsSize();
  
 private: 
  pobj::pool<rootFile> pool;
  pobj::persistent_ptr<char[]> contents;
  pobj::p<ssize_t> contents_size;
  pobj::mutex mutex;
  // For sequentialFile's Skip()
  int current_start_index;
};

struct rootFile{
  pobj::persistent_ptr<PmemFile> file;
};
} // namespace LEVELDB