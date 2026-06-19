// Debra Hensgen, 1 Raphael Finkel, 1 and Udi Manber, Two Algorithms for Barrier Synchronization International Journal
// of Parallel Programming, Vol. 17, No. 1, 1988 p. 7-8

// Cannot have callback/distinguished-thread without changing from symmetric.

typedef struct {
	TYPE LogNPROCS;
	TYPE ** intended;
	VTYPE ** AnswersOdd, ** AnswersEven;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static __thread TYPE episode = 0;

#define BARRIER_DECL
#define BARRIER_CALL block( &b, p, &episode );

static inline void block( Barrier * b, TYPE p, TYPE * episode ) {
	TYPE OddOrEven = *episode % 2;
	TYPE signal = (*episode / 2) % 2;

	for ( TYPE instance = 0; instance < b->LogNPROCS; instance += 1 ) {
		TYPE their_idx = b->intended[p][instance];		// optimization
		if ( OddOrEven == 0 ) {
			b->AnswersEven[their_idx][instance] = signal;
			Fence();
			await( b->AnswersEven[p][instance] == signal );
		} else {
			b->AnswersOdd[their_idx][instance] = signal;
			Fence();
			await( b->AnswersOdd[p][instance] == signal );
		} // if
	} // for
	*episode += 1;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	TYPE LogNPROCS = Clog2( N );

	b.LogNPROCS = LogNPROCS;
	b.intended = Allocator( N * sizeof(b.intended[0]) );
	for ( typeof(N) r = 0; r < N; r += 1 ) {
		b.intended[r] = Allocator( LogNPROCS * sizeof(b.intended[0][0]) );
	} // for

	TYPE power = 1;
	for ( typeof(LogNPROCS) c = 0; c < LogNPROCS; c += 1 ) {
		for ( typeof(N) r = 0; r < N; r += 1 ) {
			b.intended[r][c] = (power + r) % N;
		} // for
		power *= 2;
	} // for

	b.AnswersOdd =  Allocator( N * sizeof(b.AnswersOdd[0]) );
	b.AnswersEven = Allocator( N * sizeof(b.AnswersEven[0]) );
	for ( typeof(N) r = 0; r < N; r += 1 ) {
		b.AnswersOdd[r]  = Allocator( LogNPROCS * sizeof(b.AnswersOdd[0][0]) );
		b.AnswersEven[r] = Allocator( LogNPROCS * sizeof(b.AnswersEven[0][0]) );
		for ( typeof(LogNPROCS) c = 0; c < LogNPROCS; c += 1 ) {
			b.AnswersOdd[r][c] = 1;
			b.AnswersEven[r][c] = 1;
		} // for
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	for ( typeof(N) r = 0; r < N; r += 1 ) {
		free( (void *)b.intended[r] );
		free( (void *)b.AnswersEven[r] );
		free( (void *)b.AnswersOdd[r] );
	} // for
	free( b.intended );
	free( b.AnswersEven );
	free( b.AnswersOdd );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierSenseRev Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
