typedef struct {
	TYPE  CALIGN group;
	VTYPE CALIGN flag;
	VTYPE CALIGN count;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline void block( Barrier * b ) {
	TYPE negflag = ! b->flag;							// optimization (compiler probably does it)

	if ( FASTPATH( Fai( b->count, 1 ) < b->group - 1 ) ) {
		await( b->flag == negflag );
	} else {
		// CALL ACTION CALLBACK BEFORE TRIGGERING BARRIER
		b->count = 0;
		b->flag = negflag;
	} // if
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	b = (Barrier){ .group = N, .flag = false, .count = 0 };
	worker_ctor();
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierFai Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
