typedef	VTYPE CALIGN * barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( b, p );

static inline void block( barrier aa, TYPE p ) {
	TYPE aap = aa[p];									// optimization
	TYPE state = 2 * p + 2;

	if ( state < N ) {
		await( aap != aa[state] );
	} // if
	state = 2 * p + 1;
	if ( state < N ) {
		await( aap != aa[state] );
	} // if
	aap = ! aap;
	aa[p] = aap;
	await( aap == aa[0] );
} // block

//#define TESTING
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
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierTreeC Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
