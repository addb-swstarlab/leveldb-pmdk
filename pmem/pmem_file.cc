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
    contents_size0 = 0;   
    contents_size1 = 0;   
    contents_size2 = 0;   
    contents_size3 = 0;   
    current_start_index = 0;
    contents_flag = 0;
  };
  PmemFile::~PmemFile() {
    // pobj::delete_persistent<char[]>(contents0, contents_size0.get_ro());
    pobj::delete_persistent<char[]>(contents0, MAX_ARRAY_SIZE);
    pobj::delete_persistent<char[]>(contents1, MAX_ARRAY_SIZE);
    pobj::delete_persistent<char[]>(contents2, MAX_ARRAY_SIZE);
    pobj::delete_persistent<char[]>(contents3, MAX_ARRAY_SIZE);
    pool.close();
  };
  ssize_t PmemFile::Read(size_t n, char* scratch) {
    // std::cout<<""<< std::string(contents0.get()) <<"\n";
    // printf("size: %d, index: %d\n", contents_size0.get_ro(), current_start_index);

    // Already be read
    if (contents_size0.get_ro() == current_start_index) {
      return 0;
    }
    int available_indexspace = contents_size0.get_ro() - current_start_index;

    // 1) Check offset range
    // if (available_indexspace) {

    // }

    // check size
    if (n > available_indexspace) {
      // printf("[WARN1] n > available_indexspace  %d\n", available_indexspace);

      // std::string str(contents0.get());
      // strcpy(scratch, str.data());
      // strcpy(scratch, str.c_str());
      // memcpy(scratch, str.c_str(), (contents_size0.get_ro() - current_start_index));

      // contents0[current_start_index, remaining_contents) (available_indexspace size)
      memcpy(scratch, contents0.get()+current_start_index, available_indexspace);
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
      // std::string str(contents0.get());
      // std::string res = str.substr(current_start_index, n);
      // strcpy(scratch, res.data());
      // strcpy(scratch, res.c_str());

      // contents0[current_start_index, current + n] (n size)
      memcpy(scratch, contents0.get()+current_start_index, (n));
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
    // std::string str(contents0.get());
    // strcpy(scratch, str.substr(offset, n).data());

    // 1) Check offset range
    if (0 <= offset && offset < 999999) {
      unsigned long size = contents_size0.get_ro();
      // 2) Check exceptions
      if (size < offset) {
        printf("[ERROR] Read] offset is out of range \n");
        // throw exception
      }
      if (size != MAX_ARRAY_SIZE && size < offset + n) {
        printf("[ERROR] Read] Invalid access \n");
        // throw exception
      }
      unsigned long remained_space = size - offset;
      signed long required_space = n - remained_space;
      // printf("[READ DEBUG %d] %d %d\n", 0, required_space, remained_space);

      // 3) Check whether need the next contents
      if (required_space > 0) { // need next contents
        // contents0[offset, remained_space-offset) (remained_space size)
      // printf("[READ DEBUG Center1] \n");
        memcpy(scratch, contents0.get()+offset, remained_space);
        
        // 4) recursively read next contents
        Read(MAX_ARRAY_SIZE * 1, required_space, scratch + remained_space);

      } else { // just read one contents
      // printf("[READ DEBUG Center2] %lld %d\n", offset, n-offset);
        // contents0[offset, n-offset) (n size)
        // memcpy(scratch, contents0.get()+offset, (n-offset));
        memcpy(scratch, contents0.get()+offset, n);
      // printf("[READ DEBUG Center3] \n");
        // memcpy(scratch+(n-offset), "\0", sizeof(char));
        memcpy(scratch+n, "\0", sizeof(char));
      // printf("[READ DEBUG Center4] \n");
      }
      
    } else if (1000000 <= offset && offset < 1999999) {
      unsigned long size = contents_size1.get_ro();
      unsigned long offset_size = size + (MAX_ARRAY_SIZE * 1);
      // 2) Check exceptions
      if (offset_size < offset) {
        printf("[ERROR] Read] offset is out of range \n");
        // throw exception
      }
      if (offset_size != (MAX_ARRAY_SIZE * 2) && offset_size < offset + n) {
        printf("[ERROR] Read] Invalid access \n");
        // throw exception
      }
      unsigned long remained_space = offset_size - offset;
      signed long required_space = n - remained_space;
      // printf("[READ DEBUG %d] %d %d\n", 1, required_space, remained_space);

      unsigned long contents_offset = offset - (MAX_ARRAY_SIZE * 1);
      // printf("[READ DEBUG] %d %d %d\n", contents_offset, size, n-contents_offset);

      // 3) Check whether need the next contents
      if (required_space > 0) { // need next contents
        // contents1[offset, remained_space-offset) (remained_space size)
        // printf("11\n");
        memcpy(scratch, contents1.get() + contents_offset, remained_space);
        
        // 4) recursively read next contents
        Read(MAX_ARRAY_SIZE * 2, required_space, scratch + remained_space);

      } else { // just read one contents
        // printf("22 %d %d\n",contents_offset, n);
        // contents1[offset, n-offset) (n size)
        memcpy(scratch, contents1.get()+contents_offset, n);
        // printf("33\n");
        memcpy(scratch + n, "\0", sizeof(char));
        // printf("44\n");
      }

    } else if (2000000 <= offset && offset < 2999999) {
      unsigned long size = contents_size2.get_ro();
      unsigned long offset_size = size + (MAX_ARRAY_SIZE * 2);
      // 2) Check exceptions
      if (offset_size < offset) {
        printf("[ERROR] Read] offset is out of range \n");
        // throw exception
      }
      if (offset_size != (MAX_ARRAY_SIZE * 3) && offset_size < offset + n) {
        printf("[ERROR] Read] Invalid access \n");
        // throw exception
      }
      unsigned long remained_space = offset_size - offset;
      signed long required_space = n - remained_space;
      // printf("[READ DEBUG %d] %d %d\n", 2, required_space, remained_space);

      unsigned long contents_offset = offset - (MAX_ARRAY_SIZE * 2);

      // 3) Check whether need the next contents
      if (required_space > 0) { // need next contents
        // contents2[offset, remained_space-offset) (remained_space size)
        memcpy(scratch, contents2.get() + contents_offset, remained_space);
        
        // 4) recursively read next contents
        Read(MAX_ARRAY_SIZE * 3, required_space, scratch + remained_space);

      } else { // just read one contents
        // contents2[offset, n-offset) (n size)
        memcpy(scratch, contents2.get()+contents_offset, n);
        memcpy(scratch + n, "\0", sizeof(char));
      }
    } else if (3000000 <= offset && offset < 3999999) {
      unsigned long size = contents_size3.get_ro();
      unsigned long offset_size = size + (MAX_ARRAY_SIZE * 3);
      // 2) Check exceptions
      if (offset_size < offset) {
        printf("[ERROR] Read] offset is out of range \n");
        // throw exception
      }
      if (offset_size != (MAX_ARRAY_SIZE * 4) && offset_size < offset + n) {
        printf("[ERROR] Read] Invalid access \n");
        // throw exception
      }
      unsigned long remained_space = offset_size - offset;
      signed long required_space = n - remained_space;
      // printf("[READ DEBUG %d] %d %d\n", 3, required_space, remained_space);

      unsigned long contents_offset = offset - (MAX_ARRAY_SIZE * 3);

      // 3) Check whether need the next contents
      if (required_space > 0) { // need next contents
        printf("[ERROR] Read] offset + n > 4MB range\n");
        // throw exception

      } else { // just read one contents
        // contents3[offset, n-offset) (n size)
        memcpy(scratch, contents3.get()+contents_offset, n);
        memcpy(scratch + n, "\0", sizeof(char));
      }

    } else {
      printf("[ERROR] Read] offset is out of range \n");
      // throw exception

    }

    // strcpy(scratch, str.substr(offset, n).c_str());
    // printf("res: %d\n", n-offset);
    return n;
  };
  ssize_t PmemFile::Append(const char* data, size_t n) {
    if (contents_size0 == 0) {
      // printf("[START1] %d \n", n);
      pobj::transaction::exec_tx(pool, [&] {
        // 1) Make array
        contents0 = pobj::make_persistent<char[]>(MAX_ARRAY_SIZE);
        // TO DO, Only make contents0 ?
        contents1 = pobj::make_persistent<char[]>(MAX_ARRAY_SIZE);
        contents2 = pobj::make_persistent<char[]>(MAX_ARRAY_SIZE);
        contents3 = pobj::make_persistent<char[]>(MAX_ARRAY_SIZE);
        // 2) Copy data to contents
        //    Since buffer-size < 1MB, do not check contents_flag
        // contents0 = pobj::make_persistent<char[]>(n);
      // printf("22\n");
        memcpy(contents0.get(), data, n);
        // For DEBUG. Have to remove
        // memcpy(contents0.get()+n,"\n" ,sizeof(char));
        contents_size0 = n;
      }, mutex);
    }
    // Append = need recursive method 
    else {
      // printf("[START2] %d\n", n);

      // 1) Check contents_size
      switch(contents_flag) {
        case 0:
        {
          // printf("[Flag %d]\n",contents_flag);
          unsigned long original_length = contents_size0.get_ro();
          unsigned long new_length = n + contents_size0.get_ro();

          unsigned long remained_space = MAX_ARRAY_SIZE - original_length;
          signed long required_space = n - remained_space;
          // printf("[DEBUG0 %d] %d %d %d\n",contents_flag, original_length, n, required_space);

          // pobj::transaction::exec_tx(pool, [&] {
          // unsigned long original_length = contents_size0.get_ro();
          // unsigned long new_length = n + contents_size0.get_ro();

          // unsigned long remained_space = MAX_ARRAY_SIZE - original_length;
          // signed long required_space = n - remained_space;
          // printf("[DEBUG0 %d] %d %d\n",contents_flag, original_length, remained_space);

            // char original_contents[original_length];

            // memcpy(original_contents, contents0.get(), original_length);

            // memcpy(contents0.get(), original_contents, original_length);
            // if (n > remained_space) { // required_space > 0
            if (required_space > 0) { // required_space > 0
              memcpy(contents0.get()+original_length, data, remained_space);
              contents_size0 = MAX_ARRAY_SIZE;
              // printf("[DEBUG Center1 %d] %d \n",contents_flag, contents_size0.get_ro());
            } else { 
              memcpy(contents0.get()+original_length, data, n);
              contents_size0 = new_length;
              // printf("[DEBUG Center2 %d] %d \n",contents_flag, contents_size0.get_ro());
            }
            // Require appending to the next contents array
            if (required_space > 0) {
              // printf("[INFO] Require more space!! %d\n", required_space);
              ++contents_flag;
              ssize_t r = Append(data + remained_space, required_space);
            }
          // }, mutex);
          // printf("[DEBUG0 End %d] %d \n",contents_flag, contents_size0.get_ro());
          break;
        }
        case 1:
        {
          // printf("[Flag %d]\n",contents_flag);
          unsigned long original_length = contents_size1.get_ro();
          unsigned long new_length = n + contents_size1.get_ro();

          unsigned long remained_space = MAX_ARRAY_SIZE - original_length;
          signed long required_space = n - remained_space;
          // printf("[DEBUG1 %d] %d %d\n",contents_flag, original_length, n);

          // pobj::transaction::exec_tx(pool, [&] {
          // unsigned long original_length = contents_size1.get_ro();
          // unsigned long new_length = n + contents_size1.get_ro();

          // unsigned long remained_space = MAX_ARRAY_SIZE - original_length;
          // signed long required_space = n - remained_space;
          // printf("[DEBUG1 %d] %d %d\n",contents_flag, original_length, remained_space);
            // char original_contents[original_length];

            // memcpy(original_contents, contents0.get(), original_length);

            // memcpy(contents0.get(), original_contents, original_length);
            // if (n > remained_space) { // required_space > 0
            if (required_space > 0) { // required_space > 0
              memcpy(contents1.get()+original_length, data, remained_space);
              contents_size1 = MAX_ARRAY_SIZE;
              // printf("[DEBUG Center1 %d] %d \n",contents_flag, contents_size1.get_ro());
            } else { 
              memcpy(contents1.get()+original_length, data, n);
              contents_size1 = new_length;
              // printf("[DEBUG Center2 %d] %d \n",contents_flag, contents_size1.get_ro());
            }
            // Require appending to the next contents array
            if (required_space > 0) {
              // printf("[INFO] Require more space!! %d\n", required_space);
              ++contents_flag;
              ssize_t r = Append(data + remained_space, required_space);
            }
          // printf("[DEBUG1 End %d] orig %d new %d remained %d required %d\n",contents_flag, original_length, new_length, remained_space, required_space);
          // }, mutex);
          break;
        }
        case 2:
        {
          // printf("[Flag %d]\n",contents_flag);
          unsigned long original_length = contents_size2.get_ro();
          unsigned long new_length = n + contents_size2.get_ro();

          unsigned long remained_space = MAX_ARRAY_SIZE - original_length;
          signed long required_space = n - remained_space;
          // printf("[DEBUG2 %d] %d %d\n",contents_flag, original_length, n);

          // pobj::transaction::exec_tx(pool, [&] {
            // char original_contents[original_length];

            // memcpy(original_contents, contents0.get(), original_length);

            // memcpy(contents0.get(), original_contents, original_length);
            // if (n > remained_space) { // required_space > 0
            if (required_space > 0) { // required_space > 0
              memcpy(contents2.get()+original_length, data, remained_space);
              contents_size2 = MAX_ARRAY_SIZE;
            } else { 
              memcpy(contents2.get()+original_length, data, n);
              contents_size2 = new_length;
            }
            // Require appending to the next contents array
            if (required_space > 0) {
              // printf("[INFO] Require more space!! %d\n", required_space);
              ++contents_flag;
              ssize_t r = Append(data + remained_space, required_space);
            }
          // }, mutex);
          // printf("[DEBUG2 End %d] %d \n",contents_flag, contents_size2.get_ro());
          break;
        }
        case 3: 
        {
          // printf("[Flag %d]\n",contents_flag);
          // Since no more contents array, go through normal path
          // pobj::transaction::exec_tx(pool, [&] {
            unsigned long original_length = contents_size3.get_ro();
            unsigned long new_length = n + contents_size3.get_ro();

            char original_contents[original_length];
            memcpy(original_contents, contents3.get(), original_length);

            memcpy(contents3.get(), original_contents, original_length);
            memcpy(contents3.get() + original_length, data, n);
            contents_size3 = new_length;
          // }, mutex);
          break;
        }
        default:
          std::cout<<"[ERROR] undefined contents_flag! (%d)\n", contents_flag;
          // throw exception
          break;
      }
      /*
      pobj::transaction::exec_tx(pool, [&] {
        unsigned long original_length = contents_size0.get_ro();
        unsigned long new_length = n + contents_size0.get_ro();
        // printf("DEBUG1]\n");
        char original_contents[original_length];
        memcpy(original_contents, contents0.get(), original_length);
        // strcpy(original_contents, contents0.get());
        // printf("DEBUG2]\n");

        // pobj::delete_persistent<char[]>(contents0, original_length+1);
        // printf("DEBUG2.5] %d \n", new_length);

        // contents0 = pobj::make_persistent<char[]>(new_length+1);
        // printf("DEBUG3]\n");
        memcpy(contents0.get(), original_contents, original_length);
        // strcpy(contents0.get(), original_contents);
        // printf("DEBUG4]\n");
        memcpy(contents0.get()+original_length, data, n);
        // strcat(contents0.get(), data);        
        // printf("DEBUG5]\n");
        // For DEBUG. Have to remove
        // memcpy(contents0.get()+new_length, "\n", sizeof(char));
        contents_size0 = new_length;
      }, mutex);
      */
    }
    // return Status::OK();
    return n;
  }
  // ssize_t PmemFile::Append(const Slice& data) {
  //   // First append = just insert
  //   if (contents_size0 == 0) {
  //     // std::cout<<"[START1]\n";
  //     pobj::transaction::exec_tx(pool, [&] {
  //       unsigned long length = data.size();
  //       contents0 = pobj::make_persistent<char[]>(length);
  //       strcpy(contents0.get(), data.data());
  //       // printf("DEBUG Append] %c\n", contents0.get()+6);
  //       contents_size0 = length;
  //     }, mutex);
  //   }
  //   // Append = need reallocation 
  //   else {
  //     // std::cout<<"[START2]\n";
  //     pobj::transaction::exec_tx(pool, [&] {
  //       unsigned long length = data.size() + contents_size0.get_ro();
  //       char* original = contents0.get();
  //       pobj::delete_persistent<char[]>(contents0, contents_size0.get_ro());
  //       contents0 = pobj::make_persistent<char[]>(length);
  //       strcpy(contents0.get(), original);
  //       strcat(contents0.get(), data.data());
  //       contents_size0 = length;
  //     }, mutex);
  //   }
  //   // return Status::OK();
  //   return data.size();
  // };
  int PmemFile::getContentsSize() {
    return contents_size0.get_ro();
  };
}