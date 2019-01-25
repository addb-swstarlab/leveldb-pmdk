// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <deque>
#include <limits>
#include <set>
#include "leveldb/env.h"
#include "leveldb/slice.h"
#include "port/port.h"
#include "port/thread_annotations.h"
#include "util/logging.h"
#include "util/mutexlock.h"
#include "util/posix_logger.h"
#include "util/env_posix_test_helper.h"

// JH
#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <vector>
#include "pmem/pmem_file.h"
#include "libpmemobj++/mutex.hpp"
#include "env_posix.cc"

#define FILE0 "/home/hwan/pmem_dir/file0"
#define FILE1 "/home/hwan/pmem_dir/file1"
#define FILE2 "/home/hwan/pmem_dir/file2"
#define FILE3 "/home/hwan/pmem_dir/file3"
#define FILE4 "/home/hwan/pmem_dir/file4"
#define FILE5 "/home/hwan/pmem_dir/file5"
#define FILE6 "/home/hwan/pmem_dir/file6"
#define FILE7 "/home/hwan/pmem_dir/file7"
#define FILE8 "/home/hwan/pmem_dir/file8"
#define FILE9 "/home/hwan/pmem_dir/file9"
#define NUM_OF_FILES 10
#define FILESIZE (1024 * 1024  * 1024 * 1.95) // About 1.65GB
#define FILEID "file"
#define CONTENTS 1600000000 // 1.6GB
#define EACH_CONTENT 4000000
#define CONTENTS_SIZE (CONTENTS/EACH_CONTENT)   // 1.6GB / 4MB

#define OFFSET "/home/hwan/pmem_dir/offset"
#define OFFSETID "offset"
#define OFFSETS_POOL_SIZE (1024 * 1024 * 40) // 40MB 
#define OFFSETS_SIZE 4000 // 40KB 

#define EXFILE "/home/hwan/pmem_dir/exfile"
#define EXFILEID "exfile"
#define EXFILE_SIZE (1024 * 1024 * 40)
#define EXFILE_CONTENTS 10000000 // 10MB
#define EACH_EXFILE_CONTENT 2000000
#define EXFILE_CONTENTS_SIZE (EXFILE_CONTENTS/EACH_EXFILE_CONTENT)  // 10MB / 2MB

#define EXOFFSET "/home/hwan/pmem_dir/exoffset"
#define EXOFFSETID "exoffset"
#define EXOFFSETS_POOL_SIZE (1024 * 1024 * 8) // 8MB (MIN)
#define EXOFFSETS_SIZE 5 // CURRENT MANIFEST .dbtmp


// #define FILE_SIZE (1024 * 1024 * 12) // @ MB
// #define FILE_SIZE (1024 * 1024 * 1024 * 2) // @ GB

// HAVE_FDATASYNC is defined in the auto-generated port_config.h, which is
// included by port_stdcxx.h.
#if !HAVE_FDATASYNC
#define fdatasync fsync
#endif  // !HAVE_FDATASYNC

namespace leveldb {
 
namespace {

// [pmem] JH
class PmemSequentialFile : public SequentialFile {
 private:
  std::string filename_;
  pobj::pool<rootFile>* pool;
  uint32_t contents_size;
  uint32_t start_offset;
  uint32_t index;

  uint32_t current_offset;
  int fd;

 public:
  PmemSequentialFile(const std::string& fname, pobj::pool<rootFile>* pool, 
                    uint32_t start_offset, uint32_t index, int fd)
      : filename_(fname), pool(pool), start_offset(start_offset), index(index), fd(fd),
      current_offset(0) {
    pobj::persistent_ptr<rootFile> ptr = pool->get_root();
    memcpy(&contents_size, ptr->contents_size.get() + (index * sizeof(uint32_t)), 
            sizeof(uint32_t));
    if (contents_size >= EACH_CONTENT) {
      printf("[WARN][Sq-Read] contens_size is out of boundary\n");
    }
  }

  virtual ~PmemSequentialFile() {
    // pool.close();
    // printf("Delete [S]%s\n", filename_.c_str());
    if (fd >= 0) {
      // printf("Close[Seq] %s\n", filename_.c_str());
      close(fd);
    }
  }

  virtual Status Read(size_t n, Slice* result, char* scratch) {
    // std::cout<<"############## Read Seq\n";
    Status s;
    // Get info
    pobj::persistent_ptr<rootFile> ptr = pool->get_root();
    // uint32_t contents_size, current_index;

    // memcpy(&current_index, ptr->current_offset.get() + (index * sizeof(uint32_t)), 
    //         sizeof(uint32_t));
    // printf("[READ DEBUG %d] %d, %d\n",num, contents_size, current_index);
    ssize_t r;

    


    if (contents_size == current_offset) r = 0;
    else {
      // TO DO, run memcpy without buffer-size limitation
      uint32_t avaliable_indexspace = contents_size - current_offset;
      if (n > avaliable_indexspace) {
        // printf("1] %d %d\n", start_offset, current_index);
        memcpy(scratch, ptr->contents.get() + start_offset + current_offset 
            , avaliable_indexspace);
        r = avaliable_indexspace;
      } else {
        // printf("2] %d %d\n", start_offset, current_index);
        memcpy(scratch, ptr->contents.get() + start_offset + current_offset 
            , n);
        r = n;
      }
    }
    // if (contents_size == current_index) r = 0;
    // else {
    //   // TO DO, run memcpy without buffer-size limitation
    //   uint32_t avaliable_indexspace = contents_size - current_index;
    //   if (n > avaliable_indexspace) {
    //     // printf("1] %d %d\n", start_offset, current_index);
    //     memcpy(scratch, ptr->contents.get() + start_offset + current_index 
    //         , avaliable_indexspace);
    //     r = avaliable_indexspace;
    //   } else {
    //     // printf("2] %d %d\n", start_offset, current_index);
    //     memcpy(scratch, ptr->contents.get() + start_offset + current_index 
    //         , n);
    //     r = n;
    //   }
    // }
    *result = Slice(scratch, r);

    // ssize_t r = ptr->file->Read(n, scratch);
    // printf("Read %d \n", r);
    // printf("Read %d] %d '%s' %c\n",r, result->size(), result->data(), scratch[4]);
    
    // Skip r
    Skip(r);
    // printf("[Sequential][Skip]r %d\n", r);
    return s;
  }

  virtual Status Skip(uint64_t n) {
    // std::cout<<"Skip \n";
    Status s;
    // Status s = ptr->file->Skip(n);
    // update offset
    current_offset += n;
    // pobj::persistent_ptr<rootFile> ptr = pool->get_root();
    // uint32_t original_offset;
    // memcpy(&original_offset, ptr->current_offset.get() + (index * sizeof(uint32_t)),
    //         sizeof(uint32_t));
    // original_offset += n;
    // memcpy(ptr->current_offset.get() + (index * sizeof(uint32_t)), &original_offset,
    //         sizeof(uint32_t));
    return s;
  }
};

// [pmem] JH
class PmemRandomAccessFile : public RandomAccessFile {
 private:
  std::string filename_;
  pobj::pool<rootFile>* pool;
  uint32_t contents_size;
  uint32_t start_offset;
  uint32_t index;
  int fd;

  char* contents_offset;

 public:
  PmemRandomAccessFile(const std::string& fname, pobj::pool<rootFile>* pool, 
                    uint32_t start_offset, uint32_t index, int fd)
      : filename_(fname), pool(pool), start_offset(start_offset), index(index), fd(fd) {
    pobj::persistent_ptr<rootFile> ptr = pool->get_root();
    // Get contents_size
    memcpy(&contents_size, ptr->contents_size.get() + (index * sizeof(uint32_t)), 
            sizeof(uint32_t));
    if (contents_size >= EACH_CONTENT) {
      printf("[WARN][RA-Read] contens_size is out of boundary\n");
    }
    if (contents_size == 0) {
      printf("[WARN][%s] contents_size is 0.. Maybe lost some data..\n", filename_.c_str());
    }
    // Get contents start-offset
    contents_offset = ptr->contents.get();
  }

  virtual ~PmemRandomAccessFile() {
    // pool.close();
      // printf("Close1[RA] %s\n", filename_.c_str());
    if (fd >= 0) {
      printf("Close2[RA] %s\n", filename_.c_str());
      close(fd);
    }
  }

  virtual Status Read(uint64_t offset, size_t n, Slice* result,
                      char* scratch) const {
    // printf("############# %s Read %lld %lld \n", filename_.c_str(), offset, n);
    Status s;
    // Get info
    pobj::persistent_ptr<rootFile> ptr = pool->get_root();
    // pool->memcpy_persist(&contents_size, ptr->contents_size.get() + (index * sizeof(uint32_t)),
    //         sizeof(uint32_t));
    ssize_t r;

    // TO DO, run memcpy without buffer-size limitation
    uint32_t avaliable_indexspace = contents_size - offset;
    if (n > avaliable_indexspace) {
      printf("[ERROR] Randomly readable size < n\n");
      printf("filename: %s\n", filename_.c_str());
      printf("%d %d %d %d\n", contents_size, offset, avaliable_indexspace, n);
      printf("%d %d\n", start_offset, index);
      // Throw exception

      // memcpy(scratch, ptr->contents.get() + start_offset + offset 
      //     , avaliable_indexspace);
      // r = avaliable_indexspace;

      // Check, read n
      r = 0;
    } else {
      // printf("[RA DEBUG2]\n");

      // pool->memcpy_persist(scratch, ptr->contents.get() + start_offset + offset 
      //     , n);

      // memcpy(scratch, ptr->contents.get() + start_offset + offset 
      //     , n);
      r = n;
    }
    // *result = Slice(scratch, r);
    *result = Slice(ptr->contents.get() + start_offset + offset, r);
    // printf("Read %d %d %d] %d '%s' \n", offset, n, contents_size, result->size(), result->data());
    return s;
  }
};

// [pmem] JH
class PmemWritableFile : public WritableFile {
 private:
  std::string filename_;
  pobj::pool<rootFile>* pool;
  uint32_t start_offset;
  uint16_t index;
  int fd;
  uint32_t local_contents_size;

 public:
  PmemWritableFile(const std::string& fname, pobj::pool<rootFile>* pool, 
                    uint32_t start_offset, uint32_t index, int fd)
      : filename_(fname), pool(pool), start_offset(start_offset), index(index), fd(fd)
        ,local_contents_size(0) {
  }

  virtual ~PmemWritableFile() { 
    if (fd >= 0) {
      Close(); 
    }
  }

  virtual Status Append(const Slice& data) {
    // Append
    // pobj::transaction::exec_tx(*pool, [&] {
    pobj::persistent_ptr<rootFile> ptr = pool->get_root();
    // uint32_t contents_size;
    // pool->memcpy_persist(&contents_size, ptr->contents_size.get() + (index * sizeof(uint32_t)),
    //         sizeof(uint32_t));
    // pool->memcpy_persist(ptr->contents.get() + start_offset + contents_size, data.data(), 
    //         data.size());
    // memcpy(&contents_size, ptr->contents_size.get() + (index * sizeof(uint32_t)), 
    //         sizeof(uint32_t));
    memcpy(ptr->contents.get() + start_offset + local_contents_size, data.data(), 
            data.size());

    local_contents_size += data.size();
    // contents_size += data.size();

    // memcpy(ptr->contents_size.get() + (index * sizeof(uint32_t)), &contents_size,
    //         sizeof(uint32_t));
    // pool->memcpy_persist(ptr->contents_size.get() + (index * sizeof(uint32_t)), &contents_size,
    //         sizeof(uint32_t));
    // });
    return Status::OK();
  }

  virtual Status Close() {
    Status result;
    // printf("Close 1 %s\n", filename_.c_str());
    // const int r = close(fd);
    int r = 1;
    if (fd >= 0) r = close(fd);
    if (r < 0) {
      result = PosixError(filename_, errno);
    }
    pobj::persistent_ptr<rootFile> ptr = pool->get_root();
    pool->persist(ptr->contents);
    pool->memcpy_persist(ptr->contents_size.get() + (index * sizeof(uint32_t)), &local_contents_size,
            sizeof(uint32_t));
    pool->persist(ptr);
    // pool->persist(ptr->contents_size);
    fd = -1;
    // printf("Close 2 %s\n", filename_.c_str());
    return result;
  }

  virtual Status Flush() {
    return FlushBuffered();
  }

  Status SyncDirIfManifest() {
    // const char* f = filename_.c_str();
    // const char* sep = strrchr(f, '/');
    // Slice basename;
    // std::string dir;
    // if (sep == nullptr) {
    //   dir = ".";
    //   basename = f;
    // } else {
    //   dir = std::string(f, sep - f);
    //   basename = sep + 1;
    // }
    // Status s;
    // if (basename.starts_with("MANIFEST")) {
    //   int fd = open(dir.c_str(), O_RDONLY);
    //   if (fd < 0) {
    //     s = PosixError(dir, errno);
    //   } else {
    //     if (fsync(fd) < 0) {
    //       s = PosixError(dir, errno);
    //     }
    //     close(fd);
    //   }
    // }
    // return s;
    return Status::OK();
  }

  virtual Status Sync() {
    // printf("############# Sync\n");
    // Ensure new files referred to by the manifest are in the filesystem.
    // Status s = SyncDirIfManifest();
    // if (!s.ok()) {
    //   return s;
    // }
    // Status s = FlushBuffered();
    // if (s.ok()) {
    //   if (fdatasync(fd_) != 0) {
    //     s = PosixError(filename_, errno);
    //   }
    // }
    // return s;
    return Status::OK();
  }

 private:
  Status FlushBuffered() {
    // Status s = WriteRaw(buf_, pos_);
    // pos_ = 0;
    // return s;
    return Status::OK();
  }

  Status WriteRaw(const char* p, size_t n) {
    // while (n > 0) {
    //   pobj::persistent_ptr<rootFile> ptr = pool->get_root();
    //   uint32_t contents_size;
    //   memcpy(&contents_size, ptr->contents_size.get() + (index * sizeof(uint32_t)), 
    //           sizeof(uint32_t));
    //   memcpy(ptr->contents.get() + start_offset + contents_size, p, 
    //           n);
    //   contents_size += n;
    //   ssize_t r = n;
    //   memcpy(ptr->contents_size.get() + (index * sizeof(uint32_t)), &contents_size,
    //           sizeof(uint32_t));
    //   // Slice data = Slice(p, n);
    //   // printf("WriteRaw %d in %s\n", n, filename_.c_str());
    //   ssize_t r;
    //   // ssize_t r = ptr->file->Append(p, n);
    //   // printf("WriteRaw %d %s %d\n", n, p, r);
    //   // ssize_t r = ptr->file->Append(data);      
    //   // printf("WriteRaw '%c' '%c' '%c' '%c' '%c' '%c' '%c'\n", p[0], p[1], p[2], p[3], p[4], p[5], p[6]);
    //   // char buf;
    //   // memcpy(&buf, p+6, sizeof(char));
    //   // printf("WriteRaw %d '%d'\n", n, p[6]);
    //   // printf("WriteRaw %d %s %d\n", data.size(), data.data(), r);
    //   // ssize_t r = write(fd_, p, n);
    //   if (r < 0) {
    //     if (errno == EINTR) {
    //       continue;  // Retry
    //     }
    //     return PosixError(filename_, errno);
    //   }
    //   p += r;
    //   n -= r;
    // }
    return Status::OK();
  }
};

class PmemEnv : public Env {
 public:
  PmemEnv();
  virtual ~PmemEnv() {
    char msg[] = "Destroying Env::Default()\n";
    fwrite(msg, 1, sizeof(msg), stderr);

    abort();
  }

  virtual Status NewSequentialFile(const std::string& fname,
                                   SequentialFile** result) {
    // std::cout<< "NewSequentialFile "<<fname<<" \n";
    Status s;
    // 1) Get info
    int32_t num = GetFileNumber(fname);
    uint32_t index = (uint32_t)num / NUM_OF_FILES;
    pobj::pool<rootFile>* pool = GetPool(num);
    uint32_t offset = GetOffset(fname, num);
    int fd = -1;
    // Special file
    if (num == -1) {
      index = GetExtraFileNumber(fname);
      // Get renamed number
      if (index == 0) index = offset / EACH_EXFILE_CONTENT;
      // SetExtraOffset(num, offset);
      fd = open(fname.c_str(), O_RDONLY);
    } 
    // Normal file
    else {
      // Get offset
      if (num >= OFFSETS_SIZE) {
        // std::list<IndexNumPair*> *allocList = GetAllocList(num); 
        // index = GetIndexFromAllocList(allocList , (uint16_t)num);
        // offset = ((uint16_t)index * EACH_CONTENT);
        std::map<uint16_t, uint16_t> *allocMap = GetAllocMap(num); 
        index = GetIndexFromAllocMap(allocMap, (uint16_t)num);
        offset = ((uint16_t)index * EACH_CONTENT);
        // printf("[DEBUG][Sequential] %d %d %d\n", num, index, offset);
      }
    }

    *result = new PmemSequentialFile(fname, pool, offset, index, fd);
    return s;

  }

  virtual Status NewRandomAccessFile(const std::string& fname,
                                     RandomAccessFile** result) {
    // std::cout<< "NewRandomAccessFile "<<fname<<" \n";
    
    Status s;
    // 1) Get info
    int32_t num = GetFileNumber(fname);
    uint32_t index = (uint32_t)num / NUM_OF_FILES;
    pobj::pool<rootFile>* pool = GetPool(num);
    uint32_t offset = GetOffset(fname, num);
    int fd = -1;
    // Special file
    if (num == -1) {
      index = GetExtraFileNumber(fname);
      // Get renamed number
      if (index == 0) index = offset / EACH_EXFILE_CONTENT;
      fd = open(fname.c_str(), O_RDONLY);
    } 
    // Normal file
    else {
      // Get offset
      if (num >= OFFSETS_SIZE) {
    // std::cout<< "NewRandomAccessFile "<<fname<<" ";
        // std::list<IndexNumPair*> *allocList = GetAllocList(num); 
        // index = GetIndexFromAllocList(allocList , (uint16_t)num);
        // offset = ((uint16_t)index * EACH_CONTENT);
        std::map<uint16_t, uint16_t> *allocMap = GetAllocMap(num); 
        index = GetIndexFromAllocMap(allocMap, (uint16_t)num);
        offset = ((uint16_t)index * EACH_CONTENT);
        
        // std::list<uint16_t> *freeList = GetFreeList(num);
        // PrintFreeListSize(freeList);
        // PrintAllocMapSize(allocMap);
        // printf("[DEBUG][RandomAccess] %d %d %d\n", num, index, offset);
      }
    }
    *result = new PmemRandomAccessFile(fname, pool, offset, index, fd);
    return s;
  }

  virtual Status NewWritableFile(const std::string& fname,
                                 WritableFile** result) {
        // std::cout<< "NewWritableFile "<<fname<<" \n";
    Status s;
    // 1) Get info
    int32_t num = GetFileNumber(fname);
    uint16_t index = num / NUM_OF_FILES;
    pobj::pool<rootFile>* pool = GetPool(num);
    uint32_t offset = GetOffset(fname, num);
    int fd = -1;

    // Special file
    if (num == -1) {
      index = GetExtraFileNumber(fname);
      ResetFile(pool, index);
      SetExtraOffset(index, index, offset);
      fd = open(fname.c_str(), O_TRUNC | O_WRONLY | O_CREAT, 0644);
    } 
    // Normal file
    else {
      // Set offset
      if (num >= OFFSETS_SIZE) {
        // Pop from freeList
        std::list<uint16_t> *freeList = GetFreeList(num);
        // if (num == 4003 | num == 4013) {
        // if ( 4000 <= num && num <= 4009) {
        // // if ( 5000 <= num && num <= 5009) {
        //   printf("[%d]\n",num);
        //   std::list<uint16_t>::iterator iter;
        //   for (iter = freeList->begin(); iter != freeList->end(); iter++) {
        //     printf("[iter1] '%d' ", *iter);  
        //   }
          
        // if ( 4000 <= num && num <= 4009) {
        //   printf("##[%d]\n", num);
        //   PrintFile(pool);
        // }
        // if ( 4200 <= num && num <= 4209) {
        //   printf("##[%d]\n", num);
        //   PrintFile(pool);
        // }
        // Set offset
        index = PopList(freeList);
        offset = index * EACH_CONTENT;
        // if (offset == 0) printf("[DEBUG][NewWritable] '%d'\n", num);
        // printf("[DEBUG][Writable] %d %d %d\n", num, index, offset);

        // Push into allocList
        // std::list<IndexNumPair*> *allocList = GetAllocList(num);
        // PushList(allocList, index, (uint16_t)num);
        std::map<uint16_t, uint16_t> *allocMap = GetAllocMap(num);
        InsertMap(allocMap, (uint16_t)num, index);
      }
      // if ( 10 <= num && num <= 19) {
      //     printf("##[%d]\n", num);
      //     PrintFile(pool);
      //   }
      //   if ( 1000 <= num && num <= 1009) {
      //     printf("##[%d]\n", num);
      //     PrintFile(pool);
      //   }
      //   if ( num == 1001) {
      //     ResetFile(pool, 1);
      //     PrintFile(pool);
      //   }
      ResetFile(pool, index);
      SetOffset(index, (uint32_t)num, offset);

      
    }
    // Push filename list
    std::string filename = ParseFileName(fname);
    PushFileNameList(filename);
    
    *result = new PmemWritableFile(fname, pool, offset, index, fd);
    return s;
  }

  virtual Status NewAppendableFile(const std::string& fname,
                                   WritableFile** result) {
    std::cout<< "NewAppendableFile \n";
    Status s;
    // 1) Get info
    int32_t num = GetFileNumber(fname);
    uint16_t index = num / NUM_OF_FILES;
    pobj::pool<rootFile>* pool = GetPool(num);
    uint32_t offset = GetOffset(fname, num);
    int fd = -1;
      // printf("num %d offset %d\n", num, offset);
    // Special file
    if (num == -1) {
      index = GetExtraFileNumber(fname);
      ResetFile(pool, index);
      SetExtraOffset(index, index, offset);
      fd = open(fname.c_str(), O_TRUNC | O_WRONLY | O_CREAT, 0644);
    } 
    // Normal file
    else {
      // Get offset
      if (num >= OFFSETS_SIZE) {
        // Pop from freeList
        std::list<uint16_t> *freeList = GetFreeList(num);
        
        // Set offset
        index = PopList(freeList);
        offset = index * EACH_CONTENT;
        // Push into allocList
        // std::list<IndexNumPair*> *allocList = GetAllocList(num);
        // PushList(allocList, index, (uint16_t)num);
        std::map<uint16_t, uint16_t> *allocMap = GetAllocMap(num);
        InsertMap(allocMap, (uint16_t)num, index);
      }
      ResetFile(pool, index);
      SetOffset(index, (uint32_t)num, offset);
    }
    // Push filename list
    std::string filename = ParseFileName(fname);
    PushFileNameList(filename);

    *result = new PmemWritableFile(fname, pool, offset, index, fd);
    return s;
  }

  virtual bool FileExists(const std::string& fname) {
    // printf("Exists %s\n", fname.c_str());
    return access(fname.c_str(), F_OK) == 0;
  }
  // JH
  virtual int32_t GetFileNumber(const std::string& fname) {
    std::string slash = "/";
    std::string fileNumber = fname.substr( fname.rfind(slash)+1, fname.size() );

    // Filter tmp file and MANIFEST file
    if (fileNumber.find("dbtmp") != std::string::npos
        || fileNumber.find("MANIFEST") != std::string::npos
        || fileNumber.find("CURRENT") != std::string::npos) {
      return -1;
    }
    return std::atoi(fileNumber.c_str());
  }
  // For MANIFEST, dbtmp, CURRENT file
  virtual uint16_t GetExtraFileNumber(const std::string& fname) {
    std::string slash = "/"; std::string dash = "-";
    std::string fileNumber = fname.substr( fname.rfind(slash)+1, fname.size() );

    uint16_t res;
    // Filter tmp file and MANIFEST file
    if (fileNumber.find("dbtmp") != std::string::npos) {
      switch (std::atoi(fileNumber.c_str())) {
        case 1:
          res = 1;
          break;
        case 2:
          res = 2;
          break;
        default:
          abort();
          break;
      }
    } else if (fileNumber.find("MANIFEST") != std::string::npos) {
      std::string tmp = fileNumber.substr(fileNumber.find(dash)+1, fileNumber.size());
      switch (std::atoi(tmp.c_str())) {
        case 1:
          res = 3;
          break;
        case 2:
          res = 4;
          break;
        default:
          abort();
          break;
      }
    } else if (fileNumber.find("CURRENT") != std::string::npos) {
      res = 0;
    }
    return res;
  }
  // Replace fd with Vector(FileNameList)
  virtual std::string ParseFileName(const std::string& fname) {
    return fname.substr( fname.rfind("/")+1, fname.size());
  }
  virtual void PushFileNameList(const std::string& filename) {
    FileNameList.push_back(filename);
    // printf("[Push %d] %s\n", FileNameList.size(), filename.c_str());
  }
  virtual void EraseFileNameList(const std::string& filename) {
    for (std::vector<std::string>::iterator iter = FileNameList.begin(); 
          iter != FileNameList.end(); 
          iter++) {
      std::string str = *iter;
      if (str.compare(filename) == 0) {
        FileNameList.erase(iter);
        // printf("[Erase] %s\n", str.c_str()); // store only name
        break;
      }
    }
  }
  virtual void PrintFileNameList() {
    for (std::vector<std::string>::iterator iter = FileNameList.begin(); 
          iter != FileNameList.end(); 
          iter++) {
      std::string str = *iter;
      printf("##%s##\n",str.c_str());
    }
  }
  // Customized by JH
  virtual Status GetChildren(const std::string& dir,
                             std::vector<std::string>* result,
                             bool benchmark_flag) {

    result->clear();
    if (benchmark_flag) {
      DIR* d = opendir(dir.c_str());
      if (d == nullptr) {
        return PosixError(dir, errno);
      }
      struct dirent* entry;
      while ((entry = readdir(d)) != nullptr) {
        result->push_back(entry->d_name);
      }
      closedir(d);
    } else {
      // result = &FileNameList;
      for (std::vector<std::string>::iterator iter = FileNameList.begin(); 
            iter != FileNameList.end(); 
            iter++) {
        std::string str = *iter;
        result->push_back(str.c_str());
        // printf("[GC] %s\n", str.c_str()); // store only name
      }
      // PrintFileNameList();
    }
    
    return Status::OK();
  }
  // Customized by JH
  virtual Status DeleteFile(const std::string& fname, bool benchmark_flag) {
    // if (!benchmark_flag) printf("[DeleteFile] %s\n", fname.c_str());
    Status result;
    // Extra file
    if (fname.find("MANIFEST") != std::string::npos
        || fname.find("CURRENT") != std::string::npos ) {
      int32_t num = GetFileNumber(fname);
      pobj::pool<rootFile> *pool = GetPool(num);
      num = GetExtraFileNumber(fname);
      // ResetFile(pool, num);

      // Reset .dbtmp fname(num)&offset
      if (fname.find("CURRENT") != std::string::npos) {
        // printf("%d %d\n", num, offset);
        uint32_t offset = GetOffset(fname, num);
        SetExtraOffset(1, 1, offset);
        SetExtraOffset(2, 2, offset);
      }
      if (benchmark_flag) {
        if (unlink(fname.c_str()) != 0) {
          result = PosixError(fname, errno);
        }
      }
      // Erase filename list
      std::string filename = ParseFileName(fname);
      EraseFileNameList(filename);
    }
    // Normal file
    else if (fname.find(".log") != std::string::npos
              || fname.find(".ldb") != std::string::npos) {

      // if (fname.find("018") != std::string::npos)
      //   printf("[DELETE] %s\n", fname.c_str());
      if (!benchmark_flag) {
        int32_t num = GetFileNumber(fname);
        uint16_t index = num / NUM_OF_FILES;
        pobj::pool<rootFile> *pool = GetPool(num);
        uint32_t offset_reset_value = 0;
        std::list<uint16_t> *freeList = GetFreeList(num);

        // DA case (index-based)
        if (num >= OFFSETS_SIZE) {
          // std::list<IndexNumPair*> *allocList = GetAllocList(num);
          // index = GetIndexFromAllocList(allocList, (uint16_t)num);
          // PopList(allocList, index);
          std::map<uint16_t, uint16_t> *allocMap = GetAllocMap(num);
          index = GetIndexFromAllocMap(allocMap, (uint16_t)num);
          // TO DO, Check ordering
          EraseMap(allocMap, num);
          // ResetFile(pool, index);
          SetOffset(index, num, offset_reset_value);
          PushList(freeList, index);
        } 
        // Normal case (num-based)
        else {
          // ResetFile(pool, index);
          SetOffset(index, num, offset_reset_value);
          
          PushList(freeList, index);
          // printf("Push List %d\n", index);
        }
      }
      if (benchmark_flag) {
        if (unlink(fname.c_str()) != 0) {
          result = PosixError(fname, errno);
        }
      }
      std::string filename = ParseFileName(fname);
      EraseFileNameList(filename);
    } else if (fname.find("file") != std::string::npos
              || fname.find("offset") != std::string::npos) {
      // printf("[INFO] Do not delete file, offset\n");
    } else if (fname.find(".dbtmp") != std::string::npos) {
      std::string filename = ParseFileName(fname);
      EraseFileNameList(filename);
    }
    return result;
  }
  

  virtual Status CreateDir(const std::string& name) {
    Status result;
    if (mkdir(name.c_str(), 0755) != 0) {
      result = PosixError(name, errno);
    }
    return result;
  }

  virtual Status DeleteDir(const std::string& name) {
    Status result;
    if (rmdir(name.c_str()) != 0) {
      result = PosixError(name, errno);
    }
    return result;
  }

  virtual Status GetFileSize(const std::string& fname, uint64_t* size) {
    Status s;
    struct stat sbuf;
    if (stat(fname.c_str(), &sbuf) != 0) {
      *size = 0;
      s = PosixError(fname, errno);
    } else {
      *size = sbuf.st_size;
    }
    return s;
  }

  // For special-file
  virtual Status RenameFile(const std::string& src, const std::string& target) {
    Status result;

    // Just change offset's fname to 0
    if (src.find("dbtmp") != std::string::npos
        && target.find("CURRENT") != std::string::npos) {
      int32_t src_num = GetExtraFileNumber(src);
      uint32_t src_offset = GetExtraOffset(src);
      // for (int i=0; i<5; i++) {
      //   printf("[i %d]\n", i);
      //   int32_t tmp;
      //   memcpy(&tmp, exOffsets->fname.get() + (i * sizeof(uint32_t)), 
      //           sizeof(uint32_t));
      //   printf("tmp: %d\n",tmp);
      // }
      int32_t zero = 0; // CURRENT index
      memcpy(exOffsets->fname.get() + (src_num * sizeof(uint32_t)), &zero, 
              sizeof(int32_t));    
      // for (int i=0; i<5; i++) {
      //   printf("[i %d]\n", i);
      //   int32_t tmp;
      //   memcpy(&tmp, exOffsets->fname.get() + (i * sizeof(uint32_t)), 
      //           sizeof(uint32_t));
      //   printf("tmp: %d\n",tmp);
      // }
      // printf("[RENAME] %s->%s\n", src.c_str(), target.c_str());
    } 
    // Original path (LOG -> LOG.old)
    // else {
      if (rename(src.c_str(), target.c_str()) != 0) {
        result = PosixError(src, errno);
      }
    // }
    return result;
  }

  virtual Status LockFile(const std::string& fname, FileLock** lock) {
    *lock = nullptr;
    Status result;
    int fd = open(fname.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
      result = PosixError(fname, errno);
    } else if (!locks_.Insert(fname)) {
      close(fd);
      result = Status::IOError("lock " + fname, "already held by process");
    } else if (LockOrUnlock(fd, true) == -1) {
      result = PosixError("lock " + fname, errno);
      close(fd);
      locks_.Remove(fname);
    } else {
      PosixFileLock* my_lock = new PosixFileLock;
      my_lock->fd_ = fd;
      my_lock->name_ = fname;
      *lock = my_lock;
    }
    return result;
  }

  virtual Status UnlockFile(FileLock* lock) {
    PosixFileLock* my_lock = reinterpret_cast<PosixFileLock*>(lock);
    Status result;
    if (LockOrUnlock(my_lock->fd_, false) == -1) {
      result = PosixError("unlock", errno);
    }
    locks_.Remove(my_lock->name_);
    close(my_lock->fd_);
    delete my_lock;
    return result;
  }

  virtual void Schedule(void (*function)(void*), void* arg);

  virtual void StartThread(void (*function)(void* arg), void* arg);

  virtual Status GetTestDirectory(std::string* result) {
    const char* env = getenv("TEST_TMPDIR");
    if (env && env[0] != '\0') {
      *result = env;
    } else {
      char buf[100];
      snprintf(buf, sizeof(buf), "/tmp/leveldbtest-%d", int(geteuid()));
      *result = buf;
    }
    // Directory may already exist
    CreateDir(*result);
    return Status::OK();
  }

  static uint64_t gettid() {
    pthread_t tid = pthread_self();
    uint64_t thread_id = 0;
    memcpy(&thread_id, &tid, std::min(sizeof(thread_id), sizeof(tid)));
    return thread_id;
  }

  virtual Status NewLogger(const std::string& fname, Logger** result) {
    FILE* f = fopen(fname.c_str(), "w");
    if (f == nullptr) {
      *result = nullptr;
      return PosixError(fname, errno);
    } else {
      *result = new PosixLogger(f, &PmemEnv::gettid);
      return Status::OK();
    }
  }

  virtual uint64_t NowMicros() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
  }

  virtual void SleepForMicroseconds(int micros) {
    usleep(micros);
  }
  // JH
  virtual pobj::pool<rootFile>* GetPool(const uint32_t num) {
    pobj::pool<rootFile> *pool;
    switch (num % 10) { 
      case 0: pool = &filePool0; break;
      case 1: pool = &filePool1; break;
      case 2: pool = &filePool2; break;
      case 3: pool = &filePool3; break;
      case 4: pool = &filePool4; break;
      case 5: pool = &filePool5; break;
      case 6: pool = &filePool6; break;
      case 7: pool = &filePool7; break;
      case 8: pool = &filePool8; break;
      case 9: pool = &filePool9; break;
      case -1: pool = &exfilePool; break;
      default: 
        abort();
        break;
    }
    return pool;
  }
  virtual uint32_t GetOffset(const std::string& fname, const int32_t num) {
    uint32_t offset;
    // Check num range
    if (num < 0) {
      // dbtmp, MANIFEST
      offset = GetExtraOffset(fname);
    } else if (num < OFFSETS_SIZE) {
      // Pre-allocated area
      offset = (num / NUM_OF_FILES) * EACH_CONTENT; // based on 10-pools
    } 
    else {
      offset = 0;
    }
    return offset;
  }
  virtual uint32_t GetExtraOffset(const std::string& fname) {
    int32_t num = GetExtraFileNumber(fname);
    if (num == 0) {
      // Seek CURRENT file (for renaming)
      for (int i=0; i<5; i++) {
        // printf("[i %d]\n", i);
        int32_t tmp;
        memcpy(&tmp, exOffsets->fname.get() + (i * sizeof(uint32_t)), 
                sizeof(uint32_t));
        // printf("tmp: %d\n",tmp);
        if (tmp == 0) {
          num = i;
          // printf("num %d\n",num);
        }
      }
    }
    return num * EACH_EXFILE_CONTENT;
  }
  virtual void SetOffset(const uint32_t index, const uint32_t num, const uint32_t offset) {
    // offsetPool.memcpy_persist(offsets->fname.get() + (index * sizeof(uint32_t)), 
    //         &num, sizeof(uint32_t));
    // offsetPool.memcpy_persist(offsets->start_offset.get() + (index * sizeof(uint32_t)), 
    //         &offset, sizeof(uint32_t));
    memcpy(offsets->fname.get() + (index * sizeof(uint32_t)), 
            &num, sizeof(uint32_t));
    memcpy(offsets->start_offset.get() + (index * sizeof(uint32_t)), 
            &offset, sizeof(uint32_t));
  }
  virtual void SetExtraOffset(const uint32_t index, const uint32_t num, const uint32_t offset) {
    // exOffsetPool.memcpy_persist(exOffsets->fname.get() + (num * sizeof(uint32_t)), 
    //         &num, sizeof(uint32_t));
    // exOffsetPool.memcpy_persist(exOffsets->start_offset.get() + (num * sizeof(uint32_t)), 
    //         &offset, sizeof(uint32_t));
    memcpy(exOffsets->fname.get() + (index * sizeof(uint32_t)), 
            &num, sizeof(uint32_t));
    memcpy(exOffsets->start_offset.get() + (index * sizeof(uint32_t)), 
            &offset, sizeof(uint32_t));
  }
  virtual void ResetFile(pobj::pool<rootFile>* pool, const uint32_t index) {
    pobj::persistent_ptr<rootFile> ptr = pool->get_root();
    uint32_t zero = 0;
    // pool->memcpy_persist(ptr->contents_size.get() + (index * sizeof(uint32_t)), &zero, sizeof(uint32_t));
    // pool->memcpy_persist(ptr->current_offset.get() + (index * sizeof(uint32_t)), &zero, sizeof(uint32_t));
    memcpy(ptr->contents_size.get() + (index * sizeof(uint32_t)), &zero, sizeof(uint32_t));
    // memcpy(ptr->current_offset.get() + (index * sizeof(uint32_t)), &zero, sizeof(uint32_t));
  }
  // For debugging
  // virtual void PrintFile(pobj::pool<rootFile>* pool) {
  //   pobj::persistent_ptr<rootFile> ptr = pool->get_root();
  //   for (int i=0; i< CONTENTS_SIZE; i++) {
  //     uint32_t contents_size, current_offset;
  //     memcpy(&contents_size, ptr->contents_size.get() + (i * sizeof(uint32_t)),  sizeof(uint32_t));
  //     // memcpy(&current_offset, ptr->current_offset.get() + (i * sizeof(uint32_t)),  sizeof(uint32_t));
  //     printf("[%d] size: %d , offset: %d\n",i, contents_size, current_offset);
  //   }
  // }
  // For Linked-listsxv
  virtual std::list<uint16_t>* GetFreeList(const int32_t num) {
    std::list<uint16_t> *list;
    switch (num % 10) {
      case 0: list = &freeList0; break;
      case 1: list = &freeList1; break;
      case 2: list = &freeList2; break;
      case 3: list = &freeList3; break;
      case 4: list = &freeList4; break;
      case 5: list = &freeList5; break;
      case 6: list = &freeList6; break;
      case 7: list = &freeList7; break;
      case 8: list = &freeList8; break;
      case 9: list = &freeList9; break;
      default: 
        abort();
        break;
    }
    return list;
  }
  // virtual std::list<IndexNumPair*>* GetAllocList(const int32_t num) {
  //   std::list<IndexNumPair*> *list;
  //   switch (num % 10) {
  //     case 0: list = &allocList0; break;
  //     case 1: list = &allocList1; break;
  //     case 2: list = &allocList2; break;
  //     case 3: list = &allocList3; break;
  //     case 4: list = &allocList4; break;
  //     case 5: list = &allocList5; break;
  //     case 6: list = &allocList6; break;
  //     case 7: list = &allocList7; break;
  //     case 8: list = &allocList8; break;
  //     case 9: list = &allocList9; break;
  //     default: 
  //       printf("[ERROR] Invalid File name type...\n");
  //       // Throw exception
  //       break;
  //   }
  //   return list;
  // }
  virtual std::map<uint16_t, uint16_t>* GetAllocMap(const int32_t num) {
    std::map<uint16_t, uint16_t> *map;
    switch (num % 10) {
      case 0: map = &allocMap0; break;
      case 1: map = &allocMap1; break;
      case 2: map = &allocMap2; break;
      case 3: map = &allocMap3; break;
      case 4: map = &allocMap4; break;
      case 5: map = &allocMap5; break;
      case 6: map = &allocMap6; break;
      case 7: map = &allocMap7; break;
      case 8: map = &allocMap8; break;
      case 9: map = &allocMap9; break;
      default: 
        abort();
        break;
    }
    return map;
  }
  // FreeList
  virtual void PushList(std::list<uint16_t> *list, uint16_t index) {
    list->push_back(index);
  }
  virtual uint16_t PopList(std::list<uint16_t> *list) {
    assert(list->size() != 0);
    uint16_t res = list->front();
    list->pop_front();
    return res;
    // printf("[ERROR] Free List is empty..\n");
  }
  // For DEBUG
  virtual void PrintFreeListSize(std::list<uint16_t> *list) {
    printf(" freeList-size: %d ,",list->size());
  }
  // AllocList
  // virtual void PushList(std::list<IndexNumPair*> *list, 
  //                         uint16_t index, uint16_t num) {
  //   IndexNumPair* pair = new IndexNumPair(index, num);
  //   list->push_back(pair);
  // }
  // virtual void PopList(std::list<IndexNumPair*> *list, uint16_t index) {
  //   std::list<IndexNumPair*>::iterator iter;
  //   IndexNumPair* indexNumPair;
  //   bool res = false;
  //   for (iter = list->begin(); iter != list->end(); iter++) {
  //     indexNumPair = *iter;
  //     if (indexNumPair->index == index) {
  //       list->remove(indexNumPair);
  //       res = true;
  //       break;
  //     }
  //   }
  //   if (!res) {
  //     printf("[ERROR][PopAllocList] Cannot pop..\n");
  //   } else {
  //     delete indexNumPair;
  //   }
  // }
  // virtual uint16_t GetIndexFromAllocList(std::list<IndexNumPair*> *list, 
  //                                     uint16_t num) {
  //   std::list<IndexNumPair*>::iterator iter;
  //   IndexNumPair* indexNumPair;
  //   int16_t res = -1;
  //   for (iter = list->begin(); iter != list->end(); iter++) {
  //     indexNumPair = *iter;
  //     if (indexNumPair->num == num) {
  //       res = indexNumPair->index;
  //       break;
  //     }
  //   }
  //   // remove from list
  //   if (res != -1) {
  //     // list->remove(indexNumPair);
  //   } else {
  //     printf("[Error][GetIndexFromAllocList] Cannot seek index..\n");
  //     // throw exception
  //   }
  //   return res;
  // }
  // AllocMap
  virtual void InsertMap(std::map<uint16_t, uint16_t> *m, 
                          uint16_t num, uint16_t index) {
    // m->insert(std::pair<uint16_t, uint16_t>(num, index));
    m->emplace(num, index);
    // Check boundary
    assert(m->size() < OFFSETS_SIZE);
  }
  virtual void EraseMap(std::map<uint16_t, uint16_t> *m, uint16_t num) {
    int res = m->erase(num);
    assert(!res);
    // printf("[ERROR][EraseAllocMap] Cannot erase..\n");
  }
  virtual uint16_t GetIndexFromAllocMap(std::map<uint16_t, uint16_t> *m, uint16_t num) {
    std::map<uint16_t, uint16_t>::iterator iter = m->find(num);
    assert (iter != m->end());
    // printf("[ERROR][FindAllocMap] Cannot find..\n");
    return iter->second;
  }
  // For DEBUG
  virtual void PrintAllocMapSize(std::map<uint16_t, uint16_t> *m) {
    printf(" size: %d \n",m->size());
  }

 private:
  void PthreadCall(const char* label, int result) {
    if (result != 0) {
      fprintf(stderr, "pthread %s: %s\n", label, strerror(result));
      abort();
    }
  }

  // BGThread() is the body of the background thread
  void BGThread();
  static void* BGThreadWrapper(void* arg) {
    reinterpret_cast<PmemEnv*>(arg)->BGThread();
    return nullptr;
  }

  pthread_mutex_t mu_;
  pthread_cond_t bgsignal_;
  pthread_t bgthread_;
  bool started_bgthread_;

  // Entry per Schedule() call
  struct BGItem { void* arg; void (*function)(void*); };
  typedef std::deque<BGItem> BGQueue;
  BGQueue queue_;

  PosixLockTable locks_;
  Limiter mmap_limit_;
  Limiter fd_limit_;

  // JH
  // 10-filePools & ptr (400Files * 10 = 4000Files)
  pobj::pool<rootFile> filePool0; pobj::pool<rootFile> filePool1;
  pobj::pool<rootFile> filePool2; pobj::pool<rootFile> filePool3;
  pobj::pool<rootFile> filePool4; pobj::pool<rootFile> filePool5;
  pobj::pool<rootFile> filePool6; pobj::pool<rootFile> filePool7;
  pobj::pool<rootFile> filePool8; pobj::pool<rootFile> filePool9;

  // For dynamic allocation
  //  1) free list
  std::list<uint16_t> freeList0; std::list<uint16_t> freeList1;
  std::list<uint16_t> freeList2; std::list<uint16_t> freeList3;
  std::list<uint16_t> freeList4; std::list<uint16_t> freeList5;
  std::list<uint16_t> freeList6; std::list<uint16_t> freeList7;
  std::list<uint16_t> freeList8; std::list<uint16_t> freeList9;
  //  2) allocated list
  std::list<IndexNumPair*> allocList0; std::list<IndexNumPair*> allocList1;
  std::list<IndexNumPair*> allocList2; std::list<IndexNumPair*> allocList3;
  std::list<IndexNumPair*> allocList4; std::list<IndexNumPair*> allocList5;
  std::list<IndexNumPair*> allocList6; std::list<IndexNumPair*> allocList7;
  std::list<IndexNumPair*> allocList8; std::list<IndexNumPair*> allocList9;
  //  3) allocated map
  std::map<uint16_t, uint16_t> allocMap0; std::map<uint16_t, uint16_t> allocMap1;
  std::map<uint16_t, uint16_t> allocMap2; std::map<uint16_t, uint16_t> allocMap3;
  std::map<uint16_t, uint16_t> allocMap4; std::map<uint16_t, uint16_t> allocMap5;
  std::map<uint16_t, uint16_t> allocMap6; std::map<uint16_t, uint16_t> allocMap7;
  std::map<uint16_t, uint16_t> allocMap8; std::map<uint16_t, uint16_t> allocMap9;

  // 1-extra-filePool & ptr for MANIFEST, tmp file
  pobj::pool<rootFile> exfilePool;
  // pobj::persistent_ptr<rootFile> exfile;

  // 1-offsetPool (contains 4000Files )
  pobj::pool<rootOffset> offsetPool;
  pobj::persistent_ptr<rootOffset> offsets;

  // 1-extra-offsetPool (contains 2~5 Files )
  pobj::pool<rootOffset> exOffsetPool;
  pobj::persistent_ptr<rootOffset> exOffsets;

  // FileNameList
  std::vector<std::string> FileNameList;
  
};

PmemEnv::PmemEnv()
    : started_bgthread_(false),
      mmap_limit_(MaxMmaps()),
      fd_limit_(MaxOpenFiles()) {
  PthreadCall("mutex_init", pthread_mutex_init(&mu_, nullptr));
  PthreadCall("cvar_init", pthread_cond_init(&bgsignal_, nullptr));
  printf("PmemEnv start\n");
  // JH
  // tmp for initializing
  pobj::persistent_ptr<rootFile> file0;
  pobj::persistent_ptr<rootFile> file1;
  pobj::persistent_ptr<rootFile> file2;
  pobj::persistent_ptr<rootFile> file3;
  pobj::persistent_ptr<rootFile> file4;
  pobj::persistent_ptr<rootFile> file5;
  pobj::persistent_ptr<rootFile> file6;
  pobj::persistent_ptr<rootFile> file7;
  pobj::persistent_ptr<rootFile> file8;
  pobj::persistent_ptr<rootFile> file9;

  // Initialize all pools (10-files, 1-offset)
  // 1) FILE BUFFER
  if (!FileExists(FILE0)) {
    filePool0 = pobj::pool<rootFile>::create (FILE0, FILEID,
        // 2GB cause error
        // About 1.65GB
        FILESIZE , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    filePool1 = pobj::pool<rootFile>::create (FILE1, FILEID, FILESIZE , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    filePool2 = pobj::pool<rootFile>::create (FILE2, FILEID, FILESIZE , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    filePool3 = pobj::pool<rootFile>::create (FILE3, FILEID, FILESIZE , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    filePool4 = pobj::pool<rootFile>::create (FILE4, FILEID, FILESIZE , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    filePool5 = pobj::pool<rootFile>::create (FILE5, FILEID, FILESIZE , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    filePool6 = pobj::pool<rootFile>::create (FILE6, FILEID, FILESIZE , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    filePool7 = pobj::pool<rootFile>::create (FILE7, FILEID, FILESIZE , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    filePool8 = pobj::pool<rootFile>::create (FILE8, FILEID, FILESIZE , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    filePool9 = pobj::pool<rootFile>::create (FILE9, FILEID, FILESIZE , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);


    file0 = filePool0.get_root(); file1 = filePool1.get_root();
    file2 = filePool2.get_root(); file3 = filePool3.get_root();
    file4 = filePool4.get_root(); file5 = filePool5.get_root();
    file6 = filePool6.get_root(); file7 = filePool7.get_root();
    file8 = filePool8.get_root(); file9 = filePool9.get_root();

    pobj::transaction::exec_tx(filePool0, [&] {
      file0->contents = pobj::make_persistent<char[]>(CONTENTS);
      file0->contents_size = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);  
      // file0->current_offset = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);
    });
    pobj::transaction::exec_tx(filePool1, [&] {
      file1->contents = pobj::make_persistent<char[]>(CONTENTS);
      file1->contents_size = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);  
      // file1->current_offset = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);
    });
    pobj::transaction::exec_tx(filePool2, [&] {
      file2->contents = pobj::make_persistent<char[]>(CONTENTS);
      file2->contents_size = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);  
      // file2->current_offset = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);
    });
    pobj::transaction::exec_tx(filePool3, [&] {
      file3->contents = pobj::make_persistent<char[]>(CONTENTS);
      file3->contents_size = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);  
      // file3->current_offset = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);
    });
    pobj::transaction::exec_tx(filePool4, [&] {
      file4->contents = pobj::make_persistent<char[]>(CONTENTS);
      file4->contents_size = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);  
      // file4->current_offset = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);
    });
    pobj::transaction::exec_tx(filePool5, [&] {
      file5->contents = pobj::make_persistent<char[]>(CONTENTS);
      file5->contents_size = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);  
      // file5->current_offset = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);
    });
    pobj::transaction::exec_tx(filePool6, [&] {
      file6->contents = pobj::make_persistent<char[]>(CONTENTS);
      file6->contents_size = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);  
      // file6->current_offset = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);
    });
    pobj::transaction::exec_tx(filePool7, [&] {
      file7->contents = pobj::make_persistent<char[]>(CONTENTS);
      file7->contents_size = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);  
      // file7->current_offset = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);
    });
    pobj::transaction::exec_tx(filePool8, [&] {
      file8->contents = pobj::make_persistent<char[]>(CONTENTS);
      file8->contents_size = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);  
      // file8->current_offset = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);
    });
    pobj::transaction::exec_tx(filePool9, [&] {
      file9->contents = pobj::make_persistent<char[]>(CONTENTS);
      file9->contents_size = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);  
      // file9->current_offset = pobj::make_persistent<uint32_t[]>(CONTENTS_SIZE);
    });
  } else {
    filePool0 = pobj::pool<rootFile>::open (FILE0, FILEID);
    filePool1 = pobj::pool<rootFile>::open (FILE1, FILEID);
    filePool2 = pobj::pool<rootFile>::open (FILE2, FILEID);
    filePool3 = pobj::pool<rootFile>::open (FILE3, FILEID);
    filePool4 = pobj::pool<rootFile>::open (FILE4, FILEID);
    filePool5 = pobj::pool<rootFile>::open (FILE5, FILEID);
    filePool6 = pobj::pool<rootFile>::open (FILE6, FILEID);
    filePool7 = pobj::pool<rootFile>::open (FILE7, FILEID);
    filePool8 = pobj::pool<rootFile>::open (FILE8, FILEID);
    filePool9 = pobj::pool<rootFile>::open (FILE9, FILEID);

    // for (int i=0; i < CONTENTS_SIZE; i++) {
    //   ResetFile(&filePool0, i);
    //   ResetFile(&filePool1, i);
    //   ResetFile(&filePool2, i);
    //   ResetFile(&filePool3, i);
    //   ResetFile(&filePool4, i);
    //   ResetFile(&filePool5, i);
    //   ResetFile(&filePool6, i);
    //   ResetFile(&filePool7, i);
    //   ResetFile(&filePool8, i);
    //   ResetFile(&filePool9, i);
    // }

  } 
  // OFFSET BUFFER
  if (!FileExists(OFFSET)) {
    offsetPool = pobj::pool<rootOffset>::create (OFFSET, OFFSETID,
        OFFSETS_POOL_SIZE , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

    offsets = offsetPool.get_root();
    // 4000Files
    pobj::transaction::exec_tx(offsetPool, [&] {
      offsets->fname       = pobj::make_persistent<uint32_t[]>(OFFSETS_SIZE);  
      offsets->start_offset = pobj::make_persistent<uint32_t[]>(OFFSETS_SIZE);
    });

  } else {
    offsetPool = pobj::pool<rootOffset>::open (OFFSET, OFFSETID);
    offsets = offsetPool.get_root();
  }


  // tmp for initializing
  pobj::persistent_ptr<rootFile> exfile;
  // EXTRA CONTENTS BUFFER
  if (!FileExists(EXFILE)) {
    exfilePool = pobj::pool<rootFile>::create (EXFILE, EXFILEID, 
        EXFILE_SIZE, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    exfile = exfilePool.get_root();
    pobj::transaction::exec_tx(exfilePool, [&] {
      exfile->contents = pobj::make_persistent<char[]>(EXFILE_CONTENTS);
      exfile->contents_size = pobj::make_persistent<uint32_t[]>(EXFILE_CONTENTS_SIZE);  
      // exfile->current_offset = pobj::make_persistent<uint32_t[]>(EXFILE_CONTENTS_SIZE);
    });
  } else {
    exfilePool = pobj::pool<rootFile>::open (EXFILE, EXFILEID);
  }
  // EXTRA OFFSET BUFFER
  if (!FileExists(EXOFFSET)) {
    exOffsetPool = pobj::pool<rootOffset>::create (EXOFFSET, EXOFFSETID, 
        EXOFFSETS_POOL_SIZE, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    exOffsets = exOffsetPool.get_root();
    pobj::transaction::exec_tx(exOffsetPool, [&] {
      exOffsets->fname       = pobj::make_persistent<uint32_t[]>(EXOFFSETS_SIZE);  
      exOffsets->start_offset = pobj::make_persistent<uint32_t[]>(EXOFFSETS_SIZE);
    });
  } else {
    exOffsetPool = pobj::pool<rootOffset>::open (EXOFFSET, EXOFFSETID);
    exOffsets = exOffsetPool.get_root();
  }
    
  // Initialize for preventing from renaming confusion
  for (int i=0; i<5; i++) {
    SetExtraOffset(i, i, i*EACH_EXFILE_CONTENT);
  }
    printf("End all setting\n");
}

void PmemEnv::Schedule(void (*function)(void*), void* arg) {
  PthreadCall("lock", pthread_mutex_lock(&mu_));

  // Start background thread if necessary
  if (!started_bgthread_) {
    started_bgthread_ = true;
    PthreadCall(
        "create thread",
        pthread_create(&bgthread_, nullptr,  &PmemEnv::BGThreadWrapper, this));
  }

  // If the queue is currently empty, the background thread may currently be
  // waiting.
  if (queue_.empty()) {
    PthreadCall("signal", pthread_cond_signal(&bgsignal_));
  }

  // Add to priority queue
  queue_.push_back(BGItem());
  queue_.back().function = function;
  queue_.back().arg = arg;

  PthreadCall("unlock", pthread_mutex_unlock(&mu_));
}

void PmemEnv::BGThread() {
  while (true) {
    // Wait until there is an item that is ready to run
    PthreadCall("lock", pthread_mutex_lock(&mu_));
    while (queue_.empty()) {
      PthreadCall("wait", pthread_cond_wait(&bgsignal_, &mu_));
    }

    void (*function)(void*) = queue_.front().function;
    void* arg = queue_.front().arg;
    queue_.pop_front();

    PthreadCall("unlock", pthread_mutex_unlock(&mu_));
    (*function)(arg);
  }
}

void PmemEnv::StartThread(void (*function)(void* arg), void* arg) {
  pthread_t t;
  StartThreadState* state = new StartThreadState;
  state->user_function = function;
  state->arg = arg;
  PthreadCall("start thread",
              pthread_create(&t, nullptr,  &StartThreadWrapper, state));
}

}  // namespace

static pthread_once_t once = PTHREAD_ONCE_INIT;
static Env* default_env;
static void InitDefaultEnv() { default_env = new PmemEnv; }
// static void InitDefaultEnv() { default_env = new PosixEnv; }

void EnvPosixTestHelper::SetReadOnlyFDLimit(int limit) {
  assert(default_env == nullptr);
  open_read_only_file_limit = limit;
}

void EnvPosixTestHelper::SetReadOnlyMMapLimit(int limit) {
  assert(default_env == nullptr);
  mmap_limit = limit;
}

Env* Env::Default() {
  pthread_once(&once, InitDefaultEnv);
  return default_env;
}


}  // namespace leveldb
