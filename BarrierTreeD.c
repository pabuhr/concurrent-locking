typedef	VTYPE CALIGN * Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( b, p );

static inline void block( Barrier aa, TYPE p ) {
	TYPE aap = aa[p];									// optimization
	TYPE pow = 1, pd = p;

	while ( (pd & 1) == 0 && p + pow < N ) {
		await( aap != aa[p + pow] );
		pow *= 2; pd /= 2;
	} // while
	aap = ! aap;
	aa[p] = aap;
	Fence();
	await( aap == aa[0] );
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = Allocator( sizeof(typeof(b[0])) * N );
	for ( size_t i = 0; i < N; i += 1 ) {
		b[i] = false;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)b );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierTreeD Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
