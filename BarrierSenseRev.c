// Debra Hensgen, 1 Raphael Finkel, 1 and Udi Manber, Two Algorithms for Barrier Synchronization International Journal
// of Parallel Programming, Vol. 17, No. 1, 1988 p. 7-8
//
// John M. Mellor-Crummey and Michael L. Scott, Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors,
// ACM Transactions on Computer Systems, 9(1), February 1991, Fig. 8, p. 34

typedef struct {
	VTYPE CALIGN flag;
	TYPE CALIGN count;
	pthread_mutex_t lock;
} barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static barrier b CALIGN = { .flag = 0, .count = 0, .lock = PTHREAD_MUTEX_INITIALIZER };
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline void block( barrier * b ) {
	TYPE negflag = ! b->flag;
	pthread_mutex_lock( &b->lock );
	b->count += 1;
	if ( b->count < N ) {
		pthread_mutex_unlock( &b->lock );
		await( b->flag == negflag );
	} else {
		pthread_mutex_unlock( &b->lock );
		b->count = 0;
		asm( "" : : : "memory" );						// prevent compiler code movement
		b->flag = negflag;
	} // if
} // block

#define TESTING
#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierSenseRev Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
