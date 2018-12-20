/*
 *
 */

// LEVELDB
#include "leveldb/env.h"
#include "pmem/pmem_file.h"
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

// #include <string>

#define MAX_FILENUM 100000

namespace pobj = pmem::obj;

namespace leveldb {
struct rootDirectory;
class PmemDirectory {
 public:
  PmemDirectory();
  ~PmemDirectory();

  // Readable

  // Writable
  // Status Append(const Slice& data, int flag);
  Status Append(pobj::persistent_ptr<rootFile> filePtr);
  void getFile(uint64_t number);
  
 private: 
  std::string FileList;
  std::string FilePtr;
};

struct rootDirectory{
  pobj::persistent_ptr<PmemDirectory> dir;
};
} // namespace LEVELDB