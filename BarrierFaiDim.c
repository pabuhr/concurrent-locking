// Appeared first but unavailable:
// Isaac Dimitrovsky, ZLISP-A Portable Parallell LlSP Environment, Docloral Dissertation, Courant Institute, New York, NY, 1988.
//
// First citation and algorithm:
// Eric Freudenthal and Allan Gottlieb, Process Coordination with Fetch-and-Increment}, ACM, New York, NY, USA, 26(4),
// 1991, SIGPLAN Not., April, pp. 260-268, Section 4.

typedef struct {
	TYPE  CALIGN group;
	VTYPE CALIGN Syncvar;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline void block( Barrier * b ) {
	TYPE WasLess = b->Syncvar < b->group;				// optimization (compiler probably does it)

	// This algorithm uses the same variable (Syncvar) for counting and spining, which causes cache bouncing among
	// arriving and checking (spinning) threads. Hence, it is slower than FAI barriers with seperate counting and
	// checking variables.

	if ( FASTPATH( Fai( b->Syncvar, 1 ) != 2 * b->group - 1 ) ) {
		await( WasLess != (b->Syncvar < b->group) );
	} else {
		// CALL ACTION CALLBACK BEFORE TRIGGERING BARRIER
		b->Syncvar = 0;
	} // if
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	b = (Barrier){ .group = N, .Syncvar = 0 };
	worker_ctor();
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierFaiDim Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
