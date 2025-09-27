#include "BarrierCallback.h"

typedef struct {
	TYPE CALIGN group;
	VTYPE CALIGN * barrier;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b, p );

static inline bool block( Barrier * tog, TYPE p ) {
	CBSTART();											// must be first
	TYPE nz = p > 0;									// p0 special case
	await( tog->barrier[p] == nz );						// wait for p0 to arrive
	TYPE next = cycleUp( p, N );						// compute right partner
	tog->barrier[ next ] = true;						// unblock partner
	Fence();
	await( tog->barrier[p] != nz );						// wait for left partner to unblock me
	CBEND();											// must appear in safe location
	tog->barrier[ next ] = false;						// reset for next arrival
	return ! nz;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .barrier = Allocator( sizeof(typeof(b.barrier[0])) * N ) CBINIT() };
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		b.barrier[i] = false;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)b.barrier );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierRing Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
