// Debra Hensgen, 1 Raphael Finkel, 1 and Udi Manber, Two Algorithms for Barrier Synchronization International Journal
// of Parallel Programming, Vol. 17, No. 1, 1988 p. 9-10

// Cannot have callback/distinguished-thread without changing from symmetric.

typedef struct {
	TYPE LogNPROCS;
	TYPE ** intended;
	struct CALIGN {										// sequester array elements on cache lines
		VTYPE arr;
	} ** Answers;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b, p );

static inline void block( Barrier * b, TYPE p ) {
	for ( TYPE instance = 0; instance < b->LogNPROCS; instance += 1 ) {
		TYPE their_idx = b->intended[p][instance];		// optimization
		await( ! b->Answers[their_idx][instance].arr );
		b->Answers[their_idx][instance].arr = true;
		Fence();
		await( b->Answers[p][instance].arr );
		b->Answers[p][instance].arr = false;
	} // for
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	TYPE LogNPROCS = Clog2( N );

	b.LogNPROCS = LogNPROCS;
	b.intended = Allocator( N * sizeof(b.intended[0]) );
	b.Answers = Allocator( N * sizeof(b.Answers[0]) );
	for ( typeof(N) r = 0; r < N; r += 1 ) {
		b.intended[r] = Allocator( LogNPROCS * sizeof(b.intended[0][0]) );
		b.Answers[r] = Allocator( LogNPROCS * sizeof(b.Answers[0][0]) );
	} // for

	TYPE power = 1;
	for ( typeof(LogNPROCS) c = 0; c < LogNPROCS; c += 1 ) {
		for ( typeof(N) r = 0; r < N; r += 1 ) {
			b.intended[r][c] = (power + r) % N;
			b.Answers[r][c].arr = false;
		} // for
		power += power;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	for ( typeof(N) r = 0; r < N; r += 1 ) {
		free( (void *)b.intended[r] );
		free( (void *)b.Answers[r] );
	} // for
	free( b.intended );
	free( b.Answers );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierDissem Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
