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
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierFai3 Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
