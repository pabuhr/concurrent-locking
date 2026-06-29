// Simon A. Berger and Alexandros Stamatakis. 2010. Assessment of barrier implementations for fine-grain parallel
// regions on current multi-core architectures. In 2010 IEEE International Conference On Cluster Computing Workshops and
// Posters (CLUSTER WORKSHOPS). 1-8. Section III.A

#include "BarrierCallback.h"

typedef struct CALIGN {
	TYPE group;
	VTYPE CALIGN epoch;
	struct CALIGN {
		VTYPE arr;
	} * barrier;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b, p );

static inline bool block( Barrier * b, TYPE p ) {
	CBSTART();											// must be first
	TYPE currepoch = b->epoch;							// optimization (compiler probably does it)
	if ( p == 0 ) {
		WO( Fence(); );
		for ( TYPE kk = 1; kk < b->group; kk += 1 ) {	// wait for group to arrive
			await( b->barrier[kk].arr != currepoch );	// still on current epoch ?
		} // for
		CBEND();										// must appear in safe location
		WO( Fence(); );
		b->epoch = ! currepoch;							// toggle to next epoch
		return true;
	} // if
	WO( Fence(); );
	b->barrier[p].arr = ! currepoch;					// toggle to next epoch
	WO( Fence(); );
	await( b->epoch != currepoch );						// wait for next epoch
	return false;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .barrier = Allocator( sizeof(typeof(b.barrier[0])) * N ) CBINIT() };
	b.epoch = false;
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		b.barrier[i].arr = false;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)b.barrier );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierTreeTSR Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
