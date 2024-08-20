// This algorithm seems to be in the concurrency folklore (anonymous and transmitted orally), and hence, there is no
// citation.

typedef struct {
	TYPE  CALIGN group;
	VTYPE CALIGN count;
	VTYPE CALIGN generation;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline void block( Barrier * b ) {
	TYPE mygen = b->generation;
	
	if ( FASTPATH( Fai( b->count, 1 ) < b->group - 1 ) ) {
		await( b->generation != mygen );				// wait for my generation
	} else {
		// CALL ACTION CALLBACK BEFORE TRIGGERING BARRIER
		b->count = 0;									// release current generation
		b->generation += 1;								// start next generation
	} // if
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	b = (Barrier){ .group = N, .count = 0, .generation = 0 };
	worker_ctor();
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierGen Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
