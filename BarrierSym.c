// Cannot have callback/distinguished-thread without changing from symmetric to asymmetric.

typedef struct {
	TYPE CALIGN group;
	VTYPE CALIGN * barrier;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b, p );

static inline void block( Barrier * tag, TYPE p ) {
	enum { R = 4 };

	typeof(tag->barrier[0]) old = tag->barrier[p];
	tag->barrier[p] = cycleUp( old, R );
	Fence();
	// for ( size_t kk = 0; kk < b->group; kk += 1 ) { // Hesselink/Lamport
    for ( ssize_t kk = tag->group - 1; kk >= 0; kk -= 1 ) { // Hesselink/Lamport/Parsons
		await( tag->barrier[kk] != old );
	} // for
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .barrier = Allocator( sizeof(typeof(b.barrier[0])) * N ) };
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		b.barrier[i] = false;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)b.barrier );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierSym Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
