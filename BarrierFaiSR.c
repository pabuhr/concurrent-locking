// Similar to John M. Mellor-Crummey and Michael L. Scott, Algorithms for Scalable Synchronization on Shared-Memory
// Multiprocessors, ACM Transactions on Computer Systems, 9(1), February 1991, Fig. 8, p. 34

// Counts up versus down and avoids integer overflow by reseting count after each barrier completion.

// Integer overflow works because of equality testing. Requires group size of integer-size + 1 to fail.

#include "BarrierCallback.h"

typedef struct {
	TYPE CALIGN group;
	VTYPE CALIGN flag;
	VTYPE CALIGN counter;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline bool block( Barrier * b ) {
	CBSTART();											// must be first
	TYPE nepoch = ! b->flag;							// optimization (compiler probably does it)

	if ( LIKELY( Fai( b->counter, 1 ) != b->group - 1 ) ) { // not leader ?
		await( b->flag == nepoch );						// wait for quorum
		return false;
	} // if
	CBEND();											// must appear in safe location
	b->counter = 0;										// reset arrival counter
	WO( Fence(); );
	b->flag = nepoch;									// reset to next epoch
	return true;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .flag = false, .counter = 0 CBINIT() };
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierFai Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
