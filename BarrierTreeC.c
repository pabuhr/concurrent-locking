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

static inline bool block( Barrier * b, TYPE p ) {
	CBSTART();											// must be first
	TYPE leftmost = 2 * p + 1;

	if ( leftmost < b->group ) {
		if ( leftmost + 1 < b->group ) await( b->barrier[leftmost + 1] );
		await( b->barrier[leftmost] );
	} // if
	bool ret;
	if ( p != 0 ) {
		b->barrier[p] = true;
		await( ! b->barrier[p] );
		ret = false;
	} else {
		CBEND();										// must appear in safe location
		ret = true;
	} // if
	if ( leftmost < b->group ) {
		b->barrier[leftmost] = false;
		if ( leftmost + 1 < b->group ) b->barrier[leftmost + 1 ] = false;
	} // if
	return ret;
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
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierTreeC Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
