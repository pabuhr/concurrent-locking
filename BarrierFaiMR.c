// Handles integer overflow by using a small window of size G allowing equality versus relational operations.

#include "BarrierCallback.h"

typedef struct {
	TYPE CALIGN group;
	VSTYPE CALIGN low;
	VSTYPE CALIGN high;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

// The first thread stops after it has incremented high, and has not incremented barcnt. Then the second thread sees
// all threads are at the barrier but barcnt == 1.

static inline bool block( Barrier * b ) {
	CBSTART();											// must be first
	STYPE counter = Fai( b->high, 1 );

	if ( LIKELY( counter - b->low != (STYPE)(b->group - 1) ) ) { // wait ?
		await( counter - b->low < 0 );
		return false;
	} // if
	CBEND();											// must appear in safe location
	b->low = counter + 1;
	return true;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .low = 0, .high = 0 CBINIT() };
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierFai2 Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
