// Debra Hensgen, 1 Raphael Finkel, 1 and Udi Manber, Two Algorithms for Barrier Synchronization International Journal
// of Parallel Programming, Vol. 17, No. 1, 1988 p. 9-10

typedef struct {
	TYPE exponent, ** precomputed_ids;
	VTYPE ** shared_counters;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b, p );

static inline void handoff_start( Barrier * b, TYPE their_idx, TYPE round ) {
	await( ! b->shared_counters[round][their_idx] );
	b->shared_counters[round][their_idx] = true;
}

static inline void handoff_end( Barrier * b, TYPE p, TYPE round ) {
	await( b->shared_counters[round][p] );
	b->shared_counters[round][p] = false;
}

// synchronizes with one other thread, identified by their_idx
static inline void handoff( Barrier * b, TYPE p, TYPE their_idx, TYPE round ) {
	handoff_start( b, their_idx, round );
	Fence();
	handoff_end( b, p, round );   
}

static inline void block( Barrier * b, TYPE p ) {
	TYPE curr_count = 0;
	 
	while ( curr_count < b->exponent ) {
		handoff( b, p, b->precomputed_ids[curr_count][p], curr_count );
		curr_count += 1;
	} // while
}

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	b.exponent = Clog2( N );
	b.shared_counters = Allocator( b.exponent * sizeof(b.shared_counters[0]) );
	b.precomputed_ids = Allocator(b.exponent * sizeof(b.precomputed_ids[0]) );
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
	worker_ctor();
} // ctor

void __attribute__((noinline)) dtor() {
	for ( typeof(b.exponent) r = 0; r < b.exponent; r += 1 ) {
		free( (void *)b.shared_counters[r] );
		free( (void *)b.precomputed_ids[r] );
	} // for
	free( b.shared_counters );
	free( b.precomputed_ids );
	worker_dtor();
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierDissem Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
