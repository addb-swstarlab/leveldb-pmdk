/*
 * [2019.03.17][JH]
 * PMDK-based buffer class
 */

#include <iostream>
#include <fstream>
#include "util/coding.h" 
#include "pmem/pmem_buffer.h"

namespace leveldb {
  /* NOTE: DEBUG and common function for checking buffer-contents */
  void EncodeToBuffer(std::string* buffer, const Slice& key, const Slice& value) {
    PutLengthPrefixedSlice(buffer, key);
    PutLengthPrefixedSlice(buffer, value);
  }

  // DEBUG: 
  void AddToPmemBuffer(PmemBuffer* pmem_buffer, 
                                  std::string* buffer, uint64_t file_number) {
    pmem_buffer->SequentialWrite(file_number, Slice(*buffer));
  }
  uint32_t PrintKVAndReturnLength(char* buf) {
    Slice key_buffer(buf);
    Slice key, value;
    uint32_t decoded_len;
    GetLengthPrefixedSlice(&key_buffer, &key);
    decoded_len += VarintLength(key.size());
    decoded_len += key.size();
    Slice value_buffer(buf+decoded_len);
    GetLengthPrefixedSlice(&value_buffer, &value);
    decoded_len += VarintLength(value.size());
    decoded_len += value.size();

    std::string res_key(key.data(), key.size());
    std::string res_value(value.data(), value.size());

    printf("key:'%s'\n", res_key.c_str());
    printf("value:'%s'\n", res_value.c_str());

    printf("decoded_length %d\n", decoded_len);
    return decoded_len;
  }
  void GetAndPrintAll(PmemBuffer* pmem_buffer, uint64_t file_number) {
    Slice result;
    pmem_buffer->RandomRead(file_number, 0, EACH_CONTENT_SIZE, &result);
    uint32_t offset = 0;
    for (int i=0; i<10; i++) {
      printf("[%d]\n", i);
      offset += PrintKVAndReturnLength(const_cast<char *>(result.data()) + offset);
    }
  }
  uint32_t SkipNEntriesAndGetOffset(const char* buf, uint64_t file_number, uint8_t n) {
    Slice key, value;
    uint32_t skip_length = 0;
    uint32_t key_length, value_length;
    for (int i=0; i< n; i++) {
      char* tmp_buf = const_cast<char *>(buf)+skip_length;
      const char* key_ptr = GetVarint32Ptr(tmp_buf, 
                                          tmp_buf+VARINT_LENGTH, &key_length);
      uint32_t encoded_key_length = VarintLength(key_length) + key_length;
      const char* value_ptr = GetVarint32Ptr(tmp_buf+encoded_key_length, 
                                    tmp_buf+encoded_key_length+VARINT_LENGTH, 
                                    &value_length);
      uint32_t encoded_value_length = VarintLength(value_length) + value_length;
      skip_length += (encoded_key_length + encoded_value_length);
    }
    return skip_length;
  }
  
  int GetEncodedLength(const size_t key_size, const size_t value_size) {
    return VarintLength(key_size) + key_size + VarintLength(value_size) + value_size;
  }

  /* 
   * pmem-buffer DA functions 
   * NOTE: Use only allocated map 
   */
  void PmemBuffer::InsertAllocatedMap(uint64_t file_number, uint64_t index) {
    allocated_map_.emplace(file_number, index);
  }
  uint64_t PmemBuffer::AddFileAndGetNextOffset(uint64_t file_number) {
    uint64_t new_offset = current_offset;
    InsertAllocatedMap(file_number, new_offset);
    return new_offset;
  }

  /* pmdk-based buffer */
  PmemBuffer::PmemBuffer() {
    Init(BUFFER_PATH);
  }
  PmemBuffer::PmemBuffer(std::string pool_path) {
    Init(pool_path);
  }
  PmemBuffer::~PmemBuffer() {
    buffer_pool_.close();
  }
  void PmemBuffer::Init(std::string pool_path) {
    if (!file_exists(pool_path)) {
      buffer_pool_ = pobj::pool<root_pmem_buffer>::create (
                      pool_path, pool_path, 
                      (unsigned long)BUFFER_POOL_SIZE, 0666);
      root_buffer_ = buffer_pool_.get_root();

      pobj::transaction::exec_tx(buffer_pool_, [&] {
        root_buffer_->contents = 
              pobj::make_persistent<char[]>(MAX_CONTENTS_SIZE);
        root_buffer_->contents_size =
              pobj::make_persistent<uint32_t[]>(NUM_OF_CONTENTS);
      });
    } 
    // exists
    else {
      buffer_pool_ = pobj::pool<root_pmem_buffer>::open (
                      pool_path, pool_path);
      root_buffer_ = buffer_pool_.get_root();
    }
  }
  void PmemBuffer::ClearAll() {
    // Fill free_list
    for (int index=0; index<NUM_OF_CONTENTS; index++) {
      // Clear all contents_size
      // uint32_t initial_zero = 0;
      // buffer_pool_.memcpy_persist(
      //       root_buffer_->contents_size.get() + (index * sizeof(uint32_t)), 
      //       &initial_zero,
      //       sizeof(uint32_t));
    }
    // PROGRESS:
    current_offset = 0;
  }
  void PmemBuffer::SequentialWrite(uint64_t file_number, const Slice& data) {
    // Get offset(index)
    uint64_t offset = GetIndexFromAllocatedMap(&allocated_map_, file_number);
    uint32_t data_size = data.size();
    if (offset + data_size >= MAX_CONTENTS_SIZE) {
      printf("[ERROR][SequentialWrite] Out of bound.. %d %d %d\n", 
              offset, data_size, MAX_CONTENTS_SIZE);
    }
    // Sequential-Write(memcpy) from buf to specific contents offset
    DelayPmemWriteNtimes(1);
    buffer_pool_.memcpy_persist(
      root_buffer_->contents.get() + offset, 
      data.data(), 
      data_size
    );
    // Addition for updating current offset
    current_offset += data_size;
    // Set contents_size about matching offset(index)
    // buffer_pool_.memcpy_persist(
    //   root_buffer_->contents_size.get() + (index * sizeof(uint32_t)),
    //   &data_size,
    //   sizeof(uint32_t)
    // );
  }
  void PmemBuffer::RandomRead(uint64_t file_number,
                              uint64_t offset, size_t n, Slice* result) {
    // Get offset(index)
    // + Invaild check (Before read sst, it has been finished write)
    if(!CheckMapValidation(&allocated_map_, file_number)) {
      printf("[ERROR] %d is not in allocated_map...\n",file_number);
      abort();
    }
    uint32_t index = GetIndexFromAllocatedMap(&allocated_map_, 
                                                        file_number);
    // Check whether offset+size is over contents_size
    // uint32_t contents_size;
    // DelayPmemReadNtimes(1);
    // memcpy(&contents_size, 
    //       root_buffer_->contents_size.get() + (index * sizeof(uint32_t)), 
    //       sizeof(uint32_t));
    // if (offset + n > contents_size) {
    //   // Overflow
    //   printf("[WARN][PmemBuffer][RandomRead] Read overflow\n");
    //   // abort();
    // }
    // Make result Slice
    DelayPmemReadNtimes(1);
    *result = Slice( root_buffer_->contents.get() + 
                     (index) + offset, 
                     n);
  }
  /* 
   * TEST: actual function will be in GetFromPmem()
   * NOTE: Be copied from memtable.cc
   */
  std::string PmemBuffer::key(char* buf) const {
    uint32_t key_length;
    const char* key_ptr = GetVarint32Ptr(buf, buf+VARINT_LENGTH, &key_length);
    return std::string(key_ptr, key_length);
  }
  std::string PmemBuffer::value(char* buf) const {
    uint32_t key_length, value_length;
    const char* key_ptr = GetVarint32Ptr(buf, buf+VARINT_LENGTH, &key_length);
    const char* value_ptr = GetVarint32Ptr(
                        buf+key_length+VarintLength(key_length),
                        buf+key_length+VarintLength(key_length)+VARINT_LENGTH,
                        &value_length);
    return std::string(value_ptr, value_length);                    
  }
  /* Getter */
  PMEMobjpool* PmemBuffer::GetPool() {
    return buffer_pool_.get_handle();
  }
  char* PmemBuffer::GetStartOffset(uint64_t file_number) {
    uint64_t offset = AddFileAndGetNextOffset(file_number);
    return root_buffer_->contents.get() + offset;
  }


} // namespace leveldb 