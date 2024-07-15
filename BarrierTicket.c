typedef struct {
	TYPE  CALIGN group;
	VTYPE CALIGN ticket;
	VTYPE CALIGN serving;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline void block( Barrier * b ) {
	TYPE myTicket = Fai( b->ticket, 1 );

	if ( FASTPATH( b->serving - myTicket != 1 ) ) {
		TYPE myGroup = myTicket + b->group;				// optimization (compiler probably does it)
		await( myGroup < b->serving );					// wait for my group to fill
	} else {
		// CALL ACTION CALLBACK BEFORE TRIGGERING BARRIER
		b->serving += b->group;							// current group full => advance to next group
	} // if
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	b = (Barrier){ .group = N, .ticket = 0, .serving = N };
	worker_ctor();
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierTicket Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
