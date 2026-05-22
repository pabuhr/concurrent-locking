// Avoids integer overflow by reseting count after each barrier completion.

#include "BarrierCallback.h"

typedef struct {
	TYPE CALIGN group;
	VTYPE CALIGN flag;
	VTYPE CALIGN counter;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline bool block( Barrier * b ) {
	CBSTART();											// must be first
	TYPE nepoch = ! b->flag;							// optimization (compiler probably does it)

	if ( LIKELY( Fai( b->counter, 1 ) != b->group - 1 ) ) { // not leader ?
		await( b->flag == nepoch );						// wait for quorum
		return false;
	} // if
	CBEND();											// must appear in safe location
	b->counter = 0;										// reset arrival counter
	WO( Fence(); );
	b->flag = nepoch;									// reset to next epoch
	return true;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .flag = false, .counter = 0 CBINIT() };
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierFai Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
