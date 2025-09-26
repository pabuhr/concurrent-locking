// Colby Parsons

// Cannot have callback/matching without changing from symmetric to asymmetric. The problem is that the FAI
// unconditionally triggers the other thread(s) to advance. Hence, even if a thread knows it triggers the barrier, it
// has simultaneously unblocked the other threads so there is no safe point.

// Try to reduce the divide cost using libdivide using cheaper multiplication and bitshifts. 

#include "libdivide.h"

typedef struct {
	VTYPE ticket;
	struct libdivide_u64_t CALIGN fast_N;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline void block( Barrier * b ) {
	VTYPE * ticket = &b->ticket;

	TYPE done = libdivide_u64_do( Fai( *ticket, 1 ), &b->fast_N ) * N + N;
	await( *ticket >= done );
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	b.fast_N = libdivide_u64_gen( N );
	worker_ctor();
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierTicketDiv2 Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
