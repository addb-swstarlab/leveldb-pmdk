/*
 * [2019.03.17][JH]
 * PMDK-based buffer class
 */
#ifndef PMEM_BUFFER_H
#define PMEM_BUFFER_H


// #include "util/coding.h" 
#include "pmem/pmem_skiplist.h"

// use pmem with c++ bindings
namespace pobj = pmem::obj;

namespace leveldb {
  // PBuf
  struct root_pmem_buffer;
  class PmemBuffer;

  void EncodeToBuffer(std::string* buffer, const Slice& key, const Slice& value);
 
  // DEBUG:
  void AddToPmemBuffer(PmemBuffer* pmem_buffer, std::string* buffer, uint64_t file_number);
  uint32_t PrintKVAndReturnLength(char* buf);
  void GetAndPrintAll(PmemBuffer* pmem_buffer, uint64_t file_number);
  uint32_t SkipNEntriesAndGetOffset(const char* buf, uint64_t file_number, uint8_t n);
  int GetEncodedLength(const size_t key_size, const size_t value_size);

  /* pmdk-based buffer for sequential-write */
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
    char* GetStartOffset(uint64_t file_number);

    /* Dynamic Allocation */
    uint64_t AddFileAndGetNextOffset(uint64_t file_number);
    void InsertAllocatedMap(uint64_t file_number, uint64_t index);

   private:
    /* pmdk access object */
    pobj::pool<root_pmem_buffer> buffer_pool_;
    pobj::persistent_ptr<root_pmem_buffer> root_buffer_;
    uint64_t current_offset;

    /* Dynamic allocation */
    std::map<uint64_t, uint64_t> allocated_map_; // [ file_number -> index ]
  };
  /* root structure for accessing pmdk */
  struct root_pmem_buffer {
    pobj::persistent_ptr<char[]> contents;
    pobj::persistent_ptr<uint32_t[]> contents_size;
  };

} // namespace leveldb

#endif