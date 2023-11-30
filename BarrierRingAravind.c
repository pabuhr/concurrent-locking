// Alex Aravind, Barrier Synchronization: Simplified, Generalized, and Solved without Mutual Exclusion 2018 IEEE
// International Parallel and Distributed Processing Symposium Workshops (IPDPSW), Vancouver, BC, Canada, 2018,
// pp. 773-782

typedef	VTYPE CALIGN * barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL TYPE go = false;
#define BARRIER_CALL block( b, p, &go );

static inline void block( barrier b, TYPE p, TYPE * go ) {
	if ( UNLIKELY( N == 1 ) ) return;					// special case

	if ( UNLIKELY( p == 0 ) ) {
		b[p + 1] = *go;
		await( b[0] == *go );
	} else {
		if ( LIKELY( p < N - 1 ) ) {
			await( b[p] == *go );
			b[p + 1] = *go;
			await( b[0] == *go );
		} else {
			await( b[p] == *go );
			b[0] = *go;
		} // if
	} // if
	*go = ! *go;
} // block

//#define TESTING
#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = Allocator( sizeof(typeof(b[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		b[i] = true;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)b );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierRingAravind Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
