/*
 * [2019.03.20][JH]
 * PMDK-based latency functions
 */
#include "pmem/pmem_latency.h"

namespace leveldb {
/* Return the UNIX time in nanoseconds */
struct timespec nstimespec(void) {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec;
}
int nstimeCompare(const struct timespec a, const struct timespec b) {
    if (a.tv_sec != b.tv_sec) {
        if (a.tv_sec > b.tv_sec) {
            return 1;
        }
        return -1;
    }
    if (a.tv_nsec > b.tv_nsec) {
        return 1;
    } else if (a.tv_nsec < b.tv_nsec) {
        return -1;
    } else {
        return 0;
    }
}

// Delay function
void DelayPmemReadNtimes(int n) {
  // std::this_thread::sleep_for(std::chrono::nanoseconds(READ_DELAY * n));
	// struct timespec interval, remainder;
	// interval.tv_nsec = READ_DELAY * n;
	// nanosleep(&interval, &remainder);
  struct timespec base = nstimespec();
  base.tv_nsec += (READ_DELAY * n);
  while (nstimeCompare(base, nstimespec()) == 1) {
  }
}
void DelayPmemWriteNtimes(int n) {
  // std::this_thread::sleep_for(std::chrono::nanoseconds(WRITE_DELAY * n));
	// struct timespec interval, remainder;
	// interval.tv_nsec = WRITE_DELAY * n;
	// nanosleep(&interval, &remainder);
  struct timespec base = nstimespec();
  base.tv_nsec += (WRITE_DELAY * n);
  while (nstimeCompare(base, nstimespec()) == 1) {
  }
}
void* pmemobj_direct_latency(PMEMoid oid) {
    DelayPmemReadNtimes(1);
    return pmemobj_direct(oid);
}

} // namespace leveldb