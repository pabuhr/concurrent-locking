// Similar to John M. Mellor-Crummey and Michael L. Scott, Algorithms for Scalable Synchronization on Shared-Memory
// Multiprocessors, ACM Transactions on Computer Systems, 9(1), February 1991, Fig. 8, p. 34

// Counts up versus down and avoids integer overflow by reseting count after each barrier completion.

#include "BarrierCallback.h"

typedef struct {
	TYPE CALIGN group;
	VTYPE CALIGN sense;
	VTYPE CALIGN count;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline bool block( Barrier * b ) {
	CBSTART();											// must be first
	TYPE nepoch = ! b->sense;							// optimization (compiler probably does it)

	if ( LIKELY( Fai( b->count, 1 ) != b->group - 1 ) ) { // not leader ?
		await( b->sense == nepoch );						// wait for quorum
		return false;
	} // if
	CBEND();											// must appear in safe location
	b->count = 0;										// reset arrival counter
	WO( Fence(); );
	b->sense = nepoch;									// reset to next epoch
	return true;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .sense = false, .count = 0 CBINIT() };
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierFaiSR Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
