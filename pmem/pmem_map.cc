#include "pmem/pmem_map.h"
#include <sstream>

using namespace std;

namespace leveldb {
  
  /*
  // Constructor
  PmemMap::PmemMap() {
    // printf("[Start A]\n");
    pmap = new map<string, string>*[NUM_OF_PMAP];
    for (int i=0; i<NUM_OF_PMAP; i++) {
      pmap[i] = new map<string, string>;
      exist_bitmap[i] = true;
      // printf("A:%d\n",i);
      // pmap[NUM_OF_PMAP]->emplace();
    }
  }
  // Destructor
  PmemMap::~PmemMap() {
    for (int i=0; i<NUM_OF_PMAP; i++) {
      // Only free that existing indices
      if (exist_bitmap[i] == true)
        delete pmap[i];
      // printf("D:%d\n",i);
    }
    delete pmap;
    // printf("[End D]\n");
  }
  */
  // Constructor
  // PmemMap::PmemMap() {
  //   printf("[Start A]\n");
  //   // pmap = new map<string, string>*[NUM_OF_PMAP];
  //   pmap = pobj::make_persistent<map<string, string>[]>(NUM_OF_PMAP);
  //   for (int i=0; i<NUM_OF_PMAP; i++) {
  //     printf("A:%d\n",i);
  //     exist_bitmap[i] = true;
  //     // pmap[NUM_OF_PMAP]->emplace();
  //   }
  // }
  // // Destructor
  // PmemMap::~PmemMap() {
  //   pobj::delete_persistent<map<string, string>[]>(pmap, NUM_OF_PMAP);
  //   for (int i=0; i<NUM_OF_PMAP; i++) {
  //     // Only free that existing indices
  //     if (exist_bitmap[i] == true)
  //       exist_bitmap[i] = false;
  //       // printf("D:%d\n",i);
  //   }
  //   // printf("[End D]\n");
  // }
  
  // void PmemMap::Insert(uint8_t index, string &key, string &value) {
  //   pmap[index].emplace(key, value);
  // }
  // string PmemMap::Seek(uint8_t index, string &key) {
  //   map<string,string>::iterator iter = pmap[index].find(key);
  //   if (iter == pmap[index].end()) {
  //     // Not Found
  //     return "null";
  //   }
  //   return iter->second;
  // }
  // void PmemMap::Clone(uint8_t index, map<string, string> *original_map) {
  //   // Copy-assignment, Not Copy-construction
  //   pmap[index] = *original_map;
  // }
  // void PmemMap::Clear(uint8_t index) {
  //   pmap[index].clear();
  //   exist_bitmap[index] = false;
  // }

  // // DEBUG
  // void PmemMap::DebugInsert() {
  //   printf("[DEBUG] Insert some data\n");
  //   for (int i=0; i<NUM_OF_PMAP; i++) {
  //     stringstream ss;
  //     ss << i;
  //     pmap[i].emplace("a", "b "+ss.str());
  //     printf("%s-%s%d\n","a", "b ",i);
  //   }
  // }
  // void PmemMap::DebugScan() {
  //   printf("[DEBUG] Scan some data\n");
  //   for (int i=0; i<NUM_OF_PMAP; i++) {
  //     printf("1\n");
  //     map<string,string>::iterator iter = pmap[i].find("a");
  //     printf("2\n");
  //     if (iter == pmap[i].end()) {
  //       printf("[ERROR] Lost some data...\n");
  //     } else {
  //       printf("[%d]: %s-%s\n",i, iter->first.c_str(), iter->second.c_str());
  //     }
  //   }
  // }

} // namespace leveldb