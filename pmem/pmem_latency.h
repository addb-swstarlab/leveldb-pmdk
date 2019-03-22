/*
 * [2019.03.20][JH]
 * PMDK-based latency functions
 */
#include <libpmemobj.h>
#include <time.h>

// #define READ_DELAY 0
// #define WRITE_DELAY 0
#define READ_DELAY 40
#define WRITE_DELAY 400

	// DelayPmemReadNtimes(1);
#define D_RW_LATENCY(o) ({\
	D_RW(o);\
})
	// DelayPmemReadNtimes(1);
#define D_RO_LATENCY(o) ({\
	D_RO(o);\
})

namespace leveldb {

// NVM latency
void DelayPmemReadNtimes(int n); // DelayPmemReadNtimes(1);
void DelayPmemWriteNtimes(int n); // DelayPmemWriteNtimes(1);
void* pmemobj_direct_latency(PMEMoid oid);

}