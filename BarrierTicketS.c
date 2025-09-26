// Forcce integer overflow to test conditions.

#include "BarrierCallback.h"

typedef struct {
	TYPE CALIGN group;
	volatile short int CALIGN ticket;
	volatile short int CALIGN serving;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline bool block( Barrier * b ) {
	CBSTART();											// must be first
	short int myTicket = Fai( b->ticket, 1 );

	if ( LIKELY( (short int)(b->serving - myTicket) != 1 ) ) { // wait ?
		short int myGroup = myTicket + (short int)b->group; // optimization (compiler probably does it)
		await( (short int)(myGroup - b->serving) < 0 );	// wait for my group to fill
		return false;
	} // if
	CBEND();											// must appear in safe location
	b->serving += b->group;								// current group full => advance to next group
	return true;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .ticket = 0, .serving = N CBINIT() };
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierTicketS Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
