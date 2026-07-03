// Alex Aravind, Barrier Synchronization: Simplified, Generalized, and Solved without Mutual Exclusion 2018 IEEE
// International Parallel and Distributed Processing Symposium Workshops (IPDPSW), Vancouver, BC, Canada, 2018,
// pp. 773-782, Algorithm 1

// One pass token ring using thread-local storage.

#include "BarrierCallback.h"

typedef struct CALIGN {
	TYPE group;
	struct CALIGN {
		VTYPE arr;
	} * token;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL static __thread TYPE go CALIGN = false;
#define BARRIER_CALL block( &b, p, &go );

static inline bool block( Barrier * b, TYPE p, TYPE * go ) {
	CBSTART();											// must be first
	if ( UNLIKELY( b->group == 1 ) ) return true;		// special case

	bool ret = false;
	if ( UNLIKELY( p == 0 ) ) {
		b->token[p + 1].arr = *go;
		Fence();
		await( b->token[0].arr == *go );
		ret = true;
	} else {
		if ( LIKELY( p < b->group - 1 ) ) {
			await( b->token[p].arr == *go );
			b->token[p + 1].arr = *go;
			Fence();
			await( b->token[0].arr == *go );
		} else {
			await( b->token[p].arr == *go );
			CBEND();									// must appear in safe location
			b->token[0].arr = *go;
		} // if
	} // if
	*go = ! *go;
	return ret;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .token = Allocator( sizeof(typeof(b.token[0])) * N ) };
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		b.token[i].arr = true;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)b.token );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierRingAravind Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
