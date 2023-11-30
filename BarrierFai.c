typedef	struct {
	VTYPE CALIGN flag;
	TYPE CALIGN count;
} barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static barrier b CALIGN = { 0, 0 };
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline void block( barrier * b ) {
	TYPE negflag = ! b->flag;

	if ( FASTPATH( Fai( &b->count, 1 ) < N - 1 ) ) {
		await( b->flag == negflag );
	} else {
		b->count = 0;
		asm( "" : : : "memory" );						// prevent compiler code movement
		b->flag = negflag;
	} // if
} // barrier

#define TESTING
#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierFai Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
