// Colby Parsons

// Requires unbounded integers, otherwise it eventually fails on overflow because of the inequality in the waiting
// condition.  Cannot have callback/matching without changing from symmetric to asymmetric. The problem is that the FAI
// unconditionally triggers the other thread(s) to advance. Hence, even if a thread knows it triggers the barrier, it
// has simultaneously unblocked the other threads so there is no safe point.

typedef struct {
	TYPE CALIGN group;
	VTYPE CALIGN count;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline void block( Barrier * b ) {
	TYPE done = (Fai( b->count, 1 ) / b->group) * b->group + b->group; // Fai references value
	await( b->count >= done );
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .count = 0 };
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierTicketDiv Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
