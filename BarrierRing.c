// Two pass token ring.

// Cannot have callback without changing from symmetric.

#include "BarrierCallback.h"

typedef struct CALIGN {
	TYPE group;
	struct CALIGN {
		VTYPE arr;
	} * token;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b, p );

static inline bool block( Barrier * b, TYPE p ) {
	TYPE nz = p > 0;									// p0 special case
	TYPE next = cycleUp( p, N );						// compute right partner

	await( b->token[p].arr == nz );						// wait for p0 to arrive
	b->token[next].arr = true;							// unblock partner
	Fence();
	await( b->token[p].arr != nz );						// wait for left partner to unblock me
	b->token[next].arr = false;							// reset for next arrival
	return ! nz;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .token = Allocator( sizeof(typeof(b.token[0])) * N ) };
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		b.token[i].arr = false;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)b.token );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierRing Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
