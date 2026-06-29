// Cannot have callback/distinguished-thread without changing from symmetric.
// Keeps track of R generations (epochs) to prevent the re-initialization problem.

typedef struct CALIGN {
	TYPE group;
	struct CALIGN {
		VTYPE arr;
	} * barrier;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b, p );

enum { R = 3 };											// R > 2, 3 does well

static inline void block( Barrier * b, TYPE p ) {
	typeof(b->barrier[0].arr) old = b->barrier[p].arr;
	b->barrier[p].arr = cycleUp( old, R );
	Fence();
//	for ( ssize_t kk = b->group - 1; kk >= 0; kk -= 1 ) { // Hesselink/Lamport/Parsons
	for ( size_t kk = cycleUp( p, b->group ); kk != p; kk = cycleUp( kk, b->group ) ) {
		await( b->barrier[kk].arr != old );
	} // for
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	#ifdef CFMT
	if ( N == 1 ) printf( " R = %d,", R );
	#endif // CFMT
	worker_ctor();
	b = (Barrier){ .group = N, .barrier = Allocator( sizeof(typeof(b.barrier[0])) * N ) };
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		b.barrier[i].arr = 0;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)b.barrier );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierCycling Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
