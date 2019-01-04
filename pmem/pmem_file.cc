/*
 *
 */

#include "pmem/pmem_file.h"
#include <iostream>

namespace leveldb 
{
  
//   ssize_t PmemRead(PmemFile* pmemfile, size_t n, char* scratch) {
//     uint32_t contents_size0 = pmemfile->getContentsSize();
//     uint32_t current_start_index = pmemfile->getIndex();

//     // Already be read
//     if (contents_size0 == current_start_index) {
//       return 0;
//     }
    
//     int available_indexspace = contents_size0 - current_start_index;

//     // check size
//     if (n > available_indexspace) {
//       // printf("[WARN1] n > available_indexspace  %d\n", available_indexspace);

//       // contents0[current_start_index, remaining_contents) (available_indexspace size)
//       memcpy(scratch, pmemfile->getContents()+current_start_index, available_indexspace);
//       /*
//        * [Important]
//        * File Descriptor append null string at the end.
//        * But, actual size is not n+1, but n.
//        */ 
//       // TMP
//       // memcpy(scratch+available_indexspace, "\0", sizeof(char));
      
//       // Skip read area
//       PmemSkip(pmemfile, available_indexspace);
//       // return read size
//       return available_indexspace;
//     } else {
//       // printf("[WARN1] n <= available_indexspace  %d\n", available_indexspace);

//       // contents0[current_start_index, current + n] (n size)
//       memcpy(scratch, pmemfile->getContents() + current_start_index, n);
//       // TMP
//       // memcpy(scratch+n, "\0", sizeof(char));
//       PmemSkip(pmemfile, n);
//       return n;
//     }
//   };

//   Status PmemSkip(PmemFile* pmemfile, uint64_t n) {
//     uint64_t newIndex = pmemfile->getIndex() + n;
//     pmemfile->setIndex(newIndex);
//     return Status::OK();
//   };

//   ssize_t PmemRead(PmemFile* pmemfile, uint64_t offset, size_t n, char* scratch) {
//     // std::cout<<"Read offset to n \n";

//     // 1) Check offset range
//     if (0 <= offset && offset <= 3999999) {
//       uint32_t size = pmemfile->getContentsSize();

//       // 2) Check exceptions
//       if (size < offset) {
//         printf("[ERROR] Read] offset is out of range \n");
//         // throw exception
//       }
//       if (size != MAX_ARRAY_SIZE && size < offset + n) {
//         printf("[ERROR] Read] Invalid access \n");
//         // throw exception
//       }

//       unsigned long remained_space = size - offset;
//       signed long required_space = n - remained_space;
//       // printf("[READ DEBUG %d] %d %d\n", 0, required_space, remained_space);

//       // 3) Check whether need the next contents
//       if (required_space > 0) { // need next contents
//         printf("[ERROR] Required index is out of range (4MB)");
//       } else { // just read one contents
//       // printf("[READ DEBUG Center2] %lld %d\n", offset, n-offset);
//         // contents0[offset, n-offset) (n size)
//         memcpy(scratch, pmemfile->getContents()+offset, n);
//         // memcpy(scratch+n, "\0", sizeof(char));
//       }
      
//     } else {
//       printf("[ERROR] Read] offset is out of range %d\n", offset);
//       // throw exception
//     }
//     return n;
//   };

// ///////////////////////////////////////////////////////////////////////////////////

//   ssize_t PmemAppend(PmemFile* pmemfile, const char* data, size_t n) {
//     unsigned long original_length  = pmemfile->getContentsSize();
//     unsigned long new_length = n + original_length;
//     unsigned long remained_space = MAX_ARRAY_SIZE - original_length;
//     signed long required_space = n - remained_space;

//     if (required_space > 0) { // required_space > 0
//       printf("[ERROR] Index is out of range (4MB)");
//     } else { 
      
//       memcpy(pmemfile->getContents()+original_length, data, n);
//       pmemfile->setContentsSize(new_length);
//       // printf("[DEBUG Center2 %d] %d \n",contents_flag, contents_size0.get_ro());
//     }
//     return n;
//   }
//   // Con-, Destructor
//   PmemFile::PmemFile() {
//     // TO DO, transaction based on pool
//     contents0 = pobj::make_persistent<char[]>(MAX_ARRAY_SIZE);

//     contents_size0 = 0;   
//     current_start_index = 0;
//   };
//   // TO DO, transaction based on pool
//   // PmemFile::PmemFile(pobj::pool<rootFile> pool)
//   //  : pool(pool) {
//   //   contents_size0 = 0;   
//   //   current_start_index = 0;
//   // };
//   PmemFile::~PmemFile() {
//     pobj::delete_persistent<char[]>(contents0, MAX_ARRAY_SIZE);
//     // pool.close();
//   };

//   // Getter
//   const int PmemFile::getContentsSize() {
//     return contents_size0.get_ro();
//   };
//   const int PmemFile::getIndex() {
//     return current_start_index;
//   };
//   char* PmemFile::getContents() {
//     return contents0.get();
//   };
//   // Setter
//   void PmemFile::setContentsSize(uint32_t n) {
//     contents_size0 = n;
//   }
//   void PmemFile::setIndex(uint32_t n) {
//     current_start_index = n;
//   }
} // namespace leveldb