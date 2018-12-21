/*
 *
 */

#include "pmem/pmem_file.h"
#include <iostream>

namespace leveldb 
{
  PmemFile::PmemFile() {
    // pobj::transaction::exec_tx(*pool, [&] {
    // contents = nullptr;
    // contents_size = 0;   
    // }, mutex);
  };
  PmemFile::PmemFile(pobj::pool<rootFile> pool)
   : pool(pool) {
    // pobj::transaction::exec_tx(*pool, [&] {
    // contents = nullptr;
    contents_size = 0;   
    // }, mutex);
  };
  PmemFile::~PmemFile() {
    pobj::delete_persistent<char[]>(contents, contents_size.get_ro());
    pool.close();
  };
  Status PmemFile::Read(uint64_t offset, size_t n, Slice* result, char* scratch) {

    return Status::OK();
  };
  Status PmemFile::Append(const Slice& data) {
    if (contents_size == 0) {
      std::cout<<"[START1]\n";
      pobj::transaction::exec_tx(pool, [&] {
        unsigned long length = data.size();
        contents = pobj::make_persistent<char[]>(length +1);
        strcpy(contents.get(), data.data());
        contents_size = length;
      }, mutex);
    } else {
      std::cout<<"[START2]\n";
      pobj::transaction::exec_tx(pool, [&] {
        unsigned long length = data.size() + contents_size.get_ro();
        char* original = contents.get();
        contents = pobj::make_persistent<char[]>(length +1);
        strcpy(contents.get(), original);
        strcat(contents.get(), data.data());
        contents_size = length;
      }, mutex);
    }
    
    return Status::OK();
  };
  int PmemFile::getContentsSize() {
    return contents_size.get_ro();
  };
}