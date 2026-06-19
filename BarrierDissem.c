// Debra Hensgen, 1 Raphael Finkel, 1 and Udi Manber, Two Algorithms for Barrier Synchronization International Journal
// of Parallel Programming, Vol. 17, No. 1, 1988 p. 9-10

// Cannot have callback/distinguished-thread without changing from symmetric.

typedef struct {
	TYPE exponent, ** precomputed_ids;
	VTYPE ** shared_counters;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b, p );

static inline void block( Barrier * b, TYPE p ) {
	for ( TYPE round = 0; round < b->exponent; round += 1 ) {
		TYPE their_idx = b->precomputed_ids[round][p];	// optimization
		await( ! b->shared_counters[round][their_idx] );
		b->shared_counters[round][their_idx] = true;
		Fence();
		await( b->shared_counters[round][p] );
		b->shared_counters[round][p] = false;
	} // for
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b.exponent = Clog2( N );
	b.shared_counters = Allocator( b.exponent * sizeof(b.shared_counters[0]) );
	b.precomputed_ids = Allocator( b.exponent * sizeof(b.precomputed_ids[0]) );
	TYPE power = 1;
	for ( typeof(b.exponent) r = 0; r < b.exponent; r += 1 ) {
		b.shared_counters[r] = Allocator( N * sizeof(b.shared_counters[0][0]) );
		b.precomputed_ids[r] = Allocator( N * sizeof(b.precomputed_ids[0][0]) );
		for ( typeof(N) c = 0; c < N; c += 1 ) {
			b.shared_counters[r][c] = false;
			b.precomputed_ids[r][c] = (power + c) % N;
		} // for
		power *= 2;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	for ( typeof(b.exponent) r = 0; r < b.exponent; r += 1 ) {
		free( (void *)b.precomputed_ids[r] );
		free( (void *)b.shared_counters[r] );
	} // for
	free( b.precomputed_ids );
	free( b.shared_counters );
	worker_dtor();
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierDissem Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
