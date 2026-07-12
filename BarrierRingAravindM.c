// Alex Aravind, Barrier Synchronization: Simplified, Generalized, and Solved without Mutual Exclusion 2018 IEEE
// International Parallel and Distributed Processing Symposium Workshops (IPDPSW), Vancouver, BC, Canada, 2018,
// pp. 773-782, Algorithm 1

// One pass token ring modified to use lock-local storage versus thread-local in original algorithm and handle T == 1.

#include "BarrierCallback.h"

typedef struct CALIGN {
	TYPE group;
	VTYPE CALIGN go;
	struct CALIGN {
		VTYPE arr;
	} * token CALIGN;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b, p );

static inline bool block( Barrier * b, TYPE p ) {
	CBSTART();											// must be first
	TYPE mygo = b->go;

	if ( LIKELY( p != 0 ) ) await( b->token[p].arr == mygo );
	if ( LIKELY( p < b->group - 1 ) ) {
		b->token[p + 1].arr = mygo;
		Fence();
		await( b->token[0].arr == mygo );
		return false;
	} // if
	CBEND();											// must appear in safe location
	b->go = ! mygo;
	b->token[0].arr = mygo;
	return true;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .token = Allocator( sizeof(typeof(b.token[0])) * N ) };
	b.go = false;
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		b.token[i].arr = true;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)b.token );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierRingAravindM Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
