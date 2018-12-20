/*
 *
 */

#include "pmem/pmem_file.h"


namespace leveldb {
  PmemFile::PmemFile() {

  };
  PmemFile::~PmemFile() {

  };
  Status Read(uint64_t offset, size_t n, Slice* result, char* scratch) {

    return Status::OK();
  };
  Status Append(const Slice& data, int flag) {
    return Status::OK();
  };
}