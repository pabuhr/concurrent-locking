// This algorithm seems to be in the concurrency folklore (anonymous and transmitted orally), and hence, there is no
// citation.

// generation can overflow, but equality test works. 2^{32/64} + 1 threads must arrive simultaneously for failure.

#include "BarrierCallback.h"

typedef struct {
	TYPE CALIGN group;
	VTYPE CALIGN count;
	VTYPE CALIGN generation;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline bool block( Barrier * b ) {
	CBSTART();											// must be first
	TYPE mygen = b->generation;

	if ( LIKELY( Fai( b->count, 1 ) != b->group - 1 ) ) { // wait ?
		await( b->generation != mygen );				// wait for my generation
		return false;
	} // if
	CBEND();											// must appear in safe location
	b->count = 0;										// release current generation
	WO( Fence(); );
	b->generation += 1;									// start next generation
	return true;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .count = 0, .generation = 0 CBINIT() };
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierGen Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
