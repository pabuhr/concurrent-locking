// D == T

#include "BarrierCallback.h"

typedef struct CALIGN {
	TYPE group;
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
	if ( p == 0 ) {
		WO( Fence(); );
		for ( TYPE kk = 1; kk < b->group; kk += 1 ) {	// wait for children to arrive
			await( b->barrier[kk].arr );
		} // for
		CBEND();										// must appear in safe location
		WO( Fence(); );
		for ( TYPE kk = 1; kk < b->group; kk += 1 ) {	// wait for children to arrive
			b->barrier[kk].arr = false;					// release waiting threads
		} // for
		return true;
	} // if
	WO( Fence(); );
	b->barrier[p].arr = true;
	WO( Fence(); );
	await( ! b->barrier[p].arr );						// wait for p0 to see all waiting threads
	return false;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .barrier = Allocator( sizeof(typeof(b.barrier[0])) * N ) CBINIT() };
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		b.barrier[i].arr = false;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)b.barrier );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierTreeA Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
