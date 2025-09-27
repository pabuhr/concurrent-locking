// Avoids integer overflow by reseting count after each barrier completion.

#include "BarrierCallback.h"

typedef struct {
	TYPE CALIGN group;
	VTYPE CALIGN flag;
	VTYPE CALIGN count;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline bool block( Barrier * b ) {
	CBSTART();											// must be first
	TYPE negflag = ! b->flag;							// optimization (compiler probably does it)

	if ( LIKELY( Fai( b->count, 1 ) != b->group - 1 ) ) { // wait ?
		await( b->flag == negflag );
		return false;
	} // if
	CBEND();											// must appear in safe location
	b->count = 0;
	WO( Fence(); );
	b->flag = negflag;
	return true;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .flag = false, .count = 0 CBINIT() };
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierFai Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
