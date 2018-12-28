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
    int available_indexspace = contents_size.get_ro() - current_start_index;
    // check size
    if (n > available_indexspace) {
      // printf("[WARN1] n > available_indexspace  %d\n", available_indexspace);

      // std::string str(contents.get());
      // strcpy(scratch, str.data());
      // strcpy(scratch, str.c_str());
      // memcpy(scratch, str.c_str(), (contents_size.get_ro() - current_start_index));

      // contents[current_start_index, remaining_contents) (available_indexspace size)
      memcpy(scratch, contents.get()+current_start_index, available_indexspace);
      /*
       * [Important]
       * File Descriptor append null string at the end.
       * But, actual size is not n+1, but n.
       */ 
      memcpy(scratch+available_indexspace, "\0", sizeof(char));
      // Skip read area
      Skip(available_indexspace);
      // return read size
      return available_indexspace;
    } else {
      // printf("[WARN1] n <= available_indexspace  %d\n", available_indexspace);
      // std::string str(contents.get());
      // std::string res = str.substr(current_start_index, n);
      // strcpy(scratch, res.data());
      // strcpy(scratch, res.c_str());

      // contents[current_start_index, current + n] (n size)
      memcpy(scratch, contents.get()+current_start_index, (n));
      memcpy(scratch+n, "\0", sizeof(char));
      Skip(n);
      return n;
    }
  };
  Status PmemFile::Skip(uint64_t n) {
    current_start_index += n;
    return Status::OK();
  };
  ssize_t PmemFile::Read(uint64_t offset, size_t n, char* scratch) {
    // std::cout<<"Read offset to n \n";
    // std::string str(contents.get());
    // strcpy(scratch, str.substr(offset, n).data());

    // contents[offset, n-offset) (n size)
    memcpy(scratch, contents.get()+offset, (n-offset));
    memcpy(scratch+(n-offset), "\0", sizeof(char));
    // strcpy(scratch, str.substr(offset, n).c_str());
    return n-offset;
  };
  // Temp
  ssize_t PmemFile::Append(const char* data, size_t n) {
    if (contents_size == 0) {
      // printf("[START1] %d \n", n);
      pobj::transaction::exec_tx(pool, [&] {
        // contents = pobj::make_persistent<char[]>(10000);
        // contents2 = pobj::make_persistent<char[]>(10000);
        contents = pobj::make_persistent<char[]>(n+1);
        // contents = pobj::make_persistent<char[]>(n);
        // strcpy(contents.get(), data);
        // strcpy(contents.get(), data);
        memcpy(contents.get(), data, n);
        // For DEBUG. Have to remove
        memcpy(contents.get()+n,"\n" ,sizeof(char));
        contents_size = n;
      }, mutex);
    }
    // Append = need reallocation 
    else {
      // std::cout<<"[START2]\n";
      pobj::transaction::exec_tx(pool, [&] {
        unsigned long original_length = contents_size.get_ro();
        unsigned long new_length = n + contents_size.get_ro();
        printf("DEBUG1]\n");
        char original_contents[original_length];
        memcpy(original_contents, contents.get(), original_length);
        printf("DEBUG2]\n");

        // pobj::delete_persistent<char[]>(contents, contents_size.get_ro());
        pobj::delete_persistent<char[]>(contents, original_length+1);
        printf("DEBUG2.5] %d \n", new_length);
        // contents = pobj::make_persistent<char[]>(new_length);

        contents = pobj::make_persistent<char[]>(new_length+1);
        printf("DEBUG3]\n");
        memcpy(contents.get(), original_contents, original_length);
        printf("DEBUG4]\n");
        memcpy(contents.get()+original_length, data, n);
        printf("DEBUG5]\n");
        // For DEBUG. Have to remove
        memcpy(contents.get()+new_length, "\n", sizeof(char));
        // strcpy(contents.get(), original);
        // strcat(contents.get(), data);        
        contents_size = new_length;
      }, mutex);
    }
    // return Status::OK();
    return n;
  }
  ssize_t PmemFile::Append(const Slice& data) {
    // First append = just insert
    if (contents_size == 0) {
      // std::cout<<"[START1]\n";
      pobj::transaction::exec_tx(pool, [&] {
        unsigned long length = data.size();
        contents = pobj::make_persistent<char[]>(length);
        strcpy(contents.get(), data.data());
        // printf("DEBUG Append] %c\n", contents.get()+6);
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
        contents = pobj::make_persistent<char[]>(length);
        strcpy(contents.get(), original);
        strcat(contents.get(), data.data());
        contents_size = length;
      }, mutex);
    }
    // return Status::OK();
    return data.size();
  };
  int PmemFile::getContentsSize() {
    return contents_size.get_ro();
  };
}