/*
 *
 */
// PMDK
// #include <libpmemobj++/persistent_ptr.hpp>
// #include <libpmemobj++/make_persistent.hpp>
// #include <libpmemobj++/make_persistent_array.hpp>
// #include <libpmemobj++/p.hpp>
// #include <libpmemobj++/pool.hpp>
// #include <libpmemobj++/transaction.hpp>
#include <libpmemobj.h>
#include <iostream>
#include <fstream>
#include <string>
// #include <map>

#define POOL1 "PMAP_LAYOUT"
#define PMAP_PATH "/home/hwan/pmem_dir/pmap"
// #define PMAP_SIZE 1.95 * (1 << 30) // About 1.95GB
#define PMAP_SIZE 30 * (1 << 20) // About 1.95GB
// #define NUM_OF_PMAP 10
#define BUF_SIZE 3000

using namespace std;

namespace leveldb {

struct root_pmap {
 	char buf[BUF_SIZE];
};

TOID_DECLARE_ROOT(struct root_pmap);

class PmemMap {
 public:
  PmemMap();
  ~PmemMap();
  // index is SST number
  // void Insert(uint8_t index, string &key, string &value);
  // string Seek(uint8_t index, string &key);
  // void Clone(uint8_t index, map<string, string> *original_map);
  // void Clear(uint8_t index);
  
  // DEBUG
  void DebugInsert();
  void DebugScan();

 private:
  // map<string, string> **pmap;
  // pobj::persistent_ptr<map<string, string>[]> pmap;
  // bool exist_bitmap[NUM_OF_PMAP];
  // offset
};

} // namespace LEVELDB