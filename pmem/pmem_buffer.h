/*
 * [2019.03.14][JH]
 * PBAC(Persistent Byte-Adressable Compaction) buffer
 */
#ifndef PMEM_BUFFER_H
#define PMEM_BUFFER_H

#include <list>
#include <map>

#include "pmem/layout.h"

#include "util/coding.h" 

// C++
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/pool.hpp>

// TEST:
#include "pmem/pmem_skiplist.h"

// use pmem with c++ bindings
namespace pobj = pmem::obj;

namespace leveldb {

  // PBuf
  struct root_pmem_buffer;
  class PmemBuffer;

  // TEST:
  void EncodeToBuffer(std::string* buffer, const Slice& key, const Slice& value);
  void AddToPmemBuffer(PmemBuffer* pmem_buffer, std::string* buffer, uint64_t file_number);
  uint32_t PrintKVAndReturnLength(char* buf);
  void GetAndPrintAll(PmemBuffer* pmem_buffer, uint64_t file_number);
  uint32_t SkipNEntriesAndGetOffset(const char* buf, uint64_t file_number, uint8_t n);
  // std::string GetKeyFromBuffer(char* buf);
  // char* GetValueFromBuffer(char* buf, uint32_t* value_length);
  int GetEncodedLength(const size_t key_size, const size_t value_size);

  class PmemBuffer {
   public:
    PmemBuffer();
    PmemBuffer(std::string pool_path);
    ~PmemBuffer();
    void Init(std::string pool_path);
    void ClearAll();

    /* Read/Write function */
    void SequentialWrite(uint64_t file_number, const Slice& data);
    void RandomRead(uint64_t file_number, 
                    uint64_t offset, size_t n, Slice* result);
    std::string key(char* buf) const;
    std::string value(char* buf) const;

    /* Getter */
    PMEMobjpool* GetPool();
    char* SetAndGetStartOffset(uint64_t file_number);

   private:
    pobj::pool<root_pmem_buffer> buffer_pool_;
    pobj::persistent_ptr<root_pmem_buffer> root_buffer_;
    /* Dynamic allocation */
    std::list<uint64_t> free_list_;
    std::map<uint64_t, uint64_t> allocated_map_; // [ file_number -> index ]
  };
  struct root_pmem_buffer {
    pobj::persistent_ptr<char[]> contents;
    pobj::persistent_ptr<uint32_t[]> contents_size;
  };

} // namespace leveldb

#endif