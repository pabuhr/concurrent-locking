#include "BarrierCallback.h"

typedef struct CALIGN {
	TYPE group;
	struct CALIGN {
		VTYPE arr;
	} * barrier;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b, p );

#define min( v1, v2 ) (v1 < v2 ? v1 : v2)

#ifndef dary
#define dary 8 /* good default */
#endif

static inline bool block( Barrier * b, TYPE p ) {
	CBSTART();											// must be first
	TYPE left = dary * p + 1, right = min( N - 1, left + dary - 1 );

	for ( TYPE kk = left; kk <= right; kk += 1 ) {		// wait for children to arrive
		await( b->barrier[kk].arr );
	} // for
	bool ret;
	if ( p != 0 ) {										// wait for root's children
		WO( Fence(); );
		b->barrier[p].arr = true;
		WO( Fence(); );
		await( ! b->barrier[p].arr );
		ret = false;
	} else {											// root
		CBEND();										// must appear in safe location
		ret = true;
	} // if
	WO( Fence(); );
	for ( TYPE kk = left; kk <= right; kk += 1 ) {		// reset children
		b->barrier[kk].arr = false;
	} // for
	return ret;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	#ifdef CFMT
	if ( N == 1 ) printf( " %d-ary tree", dary );
	#endif // CFMT
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
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierTree -Ddary=4 Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
