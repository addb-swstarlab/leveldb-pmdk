#include "pmem/pmem_map.h"

using namespace std;

namespace leveldb {
  // Constructor
  PmemMap::PmemMap() {
    // printf("[Start A]\n");
    pmap = new map<string, string>*[NUM_OF_PMAP];
    for (int i=0; i<NUM_OF_PMAP; i++) {
      pmap[i] = new map<string, string>;
      // printf("A:%d\n",i);
      // pmap[NUM_OF_PMAP]->emplace();
    }
  }
  // Destructor
  PmemMap::~PmemMap() {
    for (int i=0; i<NUM_OF_PMAP; i++) {
      delete pmap[i];
      // printf("D:%d\n",i);
    }
    delete pmap;
    // printf("[End D]\n");
  }

} // namespace leveldb