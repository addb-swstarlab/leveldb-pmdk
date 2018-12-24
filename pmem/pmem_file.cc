/*
 *
 */

#include "pmem/pmem_file.h"
#include <iostream>

namespace leveldb 
{
  PmemFile::PmemFile() {
  };
  PmemFile::PmemFile(pobj::pool<rootFile> pool)
   : pool(pool) {
    contents_size = 0;   
    current_start_index = 0;
  };
  PmemFile::~PmemFile() {
    pobj::delete_persistent<char[]>(contents, contents_size.get_ro());
    pool.close();
  };
  ssize_t PmemFile::Read(size_t n, char* scratch) {
    // std::cout<<""<< std::string(contents.get()) <<"\n";
    // printf("size: %d, index: %d\n", contents_size.get_ro(), current_start_index);
    // Already be read
    if (contents_size.get_ro() == current_start_index) {
      return 0;
    }
    // check size
    if (n > (contents_size.get_ro() - current_start_index)) {
      std::cout<<"[WARN1] n > contents_size \n";
      std::string str(contents.get());
      strcpy(scratch, str.c_str());
      // printf("DEBUG] %s \n",scratch);
      Skip(contents_size.get_ro());
      return contents_size.get_ro();
    } else {
      std::cout<<"[WARN2] n <= contents_size\n";
      std::string str(contents.get());
      std::string res = str.substr(current_start_index, n);
      strcpy(scratch, res.c_str());
      Skip(n - current_start_index);
      return n;
    }
  };
  Status PmemFile::Skip(uint64_t n) {
    current_start_index += n;
    return Status::OK();
  };
  ssize_t PmemFile::Read(uint64_t offset, size_t n, char* scratch) {
    std::cout<<"Read offset to n \n";
    std::string str(contents.get());
    strcpy(scratch, str.substr(offset, n).c_str());
    return n-offset;
  };
  Status PmemFile::Append(const Slice& data) {
    // First append = just insert
    if (contents_size == 0) {
      // std::cout<<"[START1]\n";
      pobj::transaction::exec_tx(pool, [&] {
        unsigned long length = data.size();
        contents = pobj::make_persistent<char[]>(length +1);
        strcpy(contents.get(), data.data());
        contents_size = length;
      }, mutex);
    }
    // Append = need reallocation 
    else {
      // std::cout<<"[START2]\n";
      pobj::transaction::exec_tx(pool, [&] {
        unsigned long length = data.size() + contents_size.get_ro();
        char* original = contents.get();
        pobj::delete_persistent<char[]>(contents, contents_size.get_ro());
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