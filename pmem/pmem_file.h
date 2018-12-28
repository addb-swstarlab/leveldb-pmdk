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
#define MAX_ARRAY_SIZE 1000000 // 1MB
#define AUX_ARRAY_SIZE 500000 // 0.5MB

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
  ssize_t Append(const char* data, size_t n);
  ssize_t Append(const Slice& data);

  int getContentsSize();
  
 private: 
  pobj::pool<rootFile> pool;
  pobj::persistent_ptr<char[]> contents; // 1MB
  pobj::persistent_ptr<char[]> contents2; // 1MB
  pobj::persistent_ptr<char[]> contents3; // 0.5MB
  pobj::p<ssize_t> contents_size;
  pobj::mutex mutex;
  // For sequentialFile's Skip()
  int current_start_index;
  // For appending, Set Contents-Flag
  //  0 := contents1 (default)
  //  1 := contents2 
  //  2 := contents3 
  unsigned int contents_flag;
};

struct rootFile{
  pobj::persistent_ptr<PmemFile> file;
};
} // namespace LEVELDB