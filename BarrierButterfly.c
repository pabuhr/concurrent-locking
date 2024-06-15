// Eugene D. Brooks III, The Butterfly Barrier, International Journal of Parallel Programming, Vol. 15, No. 4, 1986, pp
// 295-307.

typedef struct {
	VTYPE ** shared_counters;
	TYPE next_power, exponent;
	bool is_power_of_two;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b, p );

static inline void handoff_start( Barrier * b, TYPE p, TYPE round ) {
	await( ! b->shared_counters[round][p] );
	b->shared_counters[round][p] = true;
} // handoff_start

static inline void handoff_end( Barrier * b, TYPE their_idx, TYPE round ) {
	await( b->shared_counters[round][their_idx] );
	b->shared_counters[round][their_idx] = false;
} // handoff_end

// synchronizes with one other thread, identified by their_idx
static inline void handoff( Barrier * b, TYPE p, TYPE their_idx, TYPE round ) {
	handoff_start( b, p, round );
	Fence();
	handoff_end( b, their_idx, round );   
} // handoff

// gets partner for the current step of the butterfly
static inline TYPE get_partner( TYPE p, TYPE curr_step ) {
	return p % (2 * curr_step) >= curr_step ? p - curr_step : p + curr_step;
} // get_partner

static inline void block( Barrier * b, TYPE p ) {
	// Gets the mirrored idx for use when N not a power of 2 used for masquerading as a missing task
	TYPE mirror_idx = b->next_power - p - 1;
	bool should_masquerade = ! b->is_power_of_two && mirror_idx > N - 1; 

	for ( TYPE curr_step = 1, curr_count = 0; curr_step < N; curr_step *= 2, curr_count += 1 ) {
		if ( should_masquerade )						// handle non power-of-two case
			handoff_start( b, mirror_idx, curr_count );

		handoff( b, p, get_partner( p, curr_step ), curr_count );

		if ( should_masquerade )						// handle non power-of-two case
			handoff_end( b, get_partner( mirror_idx, curr_step ), curr_count );
	} // for
}

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	// O(1) check if N is a power of two
	b.is_power_of_two = (N & (N - 1)) == 0;

	// exponent is x s.t. 2^x >= N > 2^(x-1)
	b.exponent = Clog2( N );
	b.next_power = pow( 2, b.exponent );
	b.shared_counters = Allocator( b.exponent * sizeof(b.shared_counters[0]) );
	for ( TYPE r = 0; r < b.exponent; r += 1 ) {
		b.shared_counters[r] = Allocator( b.next_power * sizeof(b.shared_counters[0][0]));
		for ( TYPE c = 0; c < b.next_power; c += 1 ) {
			b.shared_counters[r][c] = false;
		} // for
	} // for
	worker_ctor();
} // ctor

void __attribute__((noinline)) dtor() {
	for ( TYPE r = 0; r < b.exponent; r += 1 ) {
		free( (void *)b.shared_counters[r] );
	} // for
	free( b.shared_counters );
	worker_dtor();
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierButterfly Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
