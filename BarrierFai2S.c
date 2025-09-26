// Forcce integer overflow to test conditions.

#include "BarrierCallback.h"

typedef struct {
	TYPE CALIGN group;
	volatile short int CALIGN low;
	volatile short int CALIGN high;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline bool block( Barrier * b ) {
	CBSTART();											// must be first
	short int ticket = Fai( b->high, 1 );

	if ( LIKELY( (short int)(ticket + 1) != (short int)(b->low + b->group) ) ) { // wait ?
		await( (short int)(ticket - b->low) < 0 );
		return false;
	} // if
	CBEND();											// must appear in safe location
	b->low += b->group;
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
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierFai2S Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
