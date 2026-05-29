// Alex Aravind, Barrier Synchronization: Simplified, Generalized, and Solved without Mutual Exclusion 2018 IEEE
// International Parallel and Distributed Processing Symposium Workshops (IPDPSW), Vancouver, BC, Canada, 2018,
// pp. 773-782, Algorithm 1

// One pass token ring using thread-local storage.

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

#define BARRIER_DECL static __thread TYPE go CALIGN = false;
#define BARRIER_CALL block( &b, p, &go );

static inline void block( Barrier * b, TYPE p, TYPE * go ) {
	CBSTART();											// must be first
	if ( UNLIKELY( b->group == 1 ) ) return;			// special case

	if ( UNLIKELY( p == 0 ) ) {
		b->barrier[p + 1].arr = *go;
		Fence();
		await( b->barrier[0].arr == *go );
	} else {
		if ( LIKELY( p < b->group - 1 ) ) {
			await( b->barrier[p].arr == *go );
			b->barrier[p + 1].arr = *go;
			Fence();
			await( b->barrier[0].arr == *go );
		} else {
			await( b->barrier[p].arr == *go );
			CBEND();									// must appear in safe location
			b->barrier[0].arr = *go;
			Fence();
		} // if
	} // if
	*go = ! *go;
	Fence();
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .barrier = Allocator( sizeof(typeof(b.barrier[0])) * N ) };
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		b.barrier[i].arr = true;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)b.barrier );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierRingAravind Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
