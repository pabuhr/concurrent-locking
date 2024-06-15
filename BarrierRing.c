typedef	VTYPE CALIGN * Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( b, p );

static inline void block( Barrier tog, TYPE p ) {
	TYPE state = p > 0;									// optimization
	await( tog[p] == state );
	tog[ cycleUp( p, N ) ] = true;
	Fence();
	await( tog[p] != state );
	tog[ cycleUp( p, N ) ] = false;
	Fence();
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = Allocator( sizeof(typeof(b[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		b[i] = false;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)b );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierRing Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
