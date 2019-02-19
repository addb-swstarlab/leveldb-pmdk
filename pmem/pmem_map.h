/*
 *
 */

// LEVELDB
// #include "leveldb/env.h"

// PMDK
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <map>

#define POOL1 "pmap"
#define PMAP_PATH "/home/hwan/pmem_dir/pmap"
#define PMAP_SIZE 1.95 * (1 << 30) // About 1.95GB
#define NUM_OF_PMAP 500

namespace pobj = pmem::obj;
using namespace std;

namespace leveldb {

class PmemMap {
 public:
  PmemMap();
  ~PmemMap();
  void Insert();
  void Seek();

 private:
  map<string, string> **pmap;
};

struct root {
  pobj::persistent_ptr<PmemMap> pmap_ptr;
};
} // namespace LEVELDB