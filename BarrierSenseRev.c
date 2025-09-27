// Debra Hensgen, 1 Raphael Finkel, 1 and Udi Manber, Two Algorithms for Barrier Synchronization International Journal
// of Parallel Programming, Vol. 17, No. 1, 1988 p. 7-8
//
// John M. Mellor-Crummey and Michael L. Scott, Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors,
// ACM Transactions on Computer Systems, 9(1), February 1991, Fig. 8, p. 34

#include "BarrierCallback.h"

typedef struct {
	TYPE CALIGN group;
	VTYPE CALIGN flag;
	VTYPE CALIGN count;
	pthread_mutex_t lock;
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
	pthread_mutex_lock( &b->lock );

	b->count += 1;
	if ( LIKELY( b->count < b->group ) ) {
		pthread_mutex_unlock( &b->lock );
		await( b->flag == negflag );
		return false;
	} // if
	CBEND();											// must appear in safe location
	b->count = 0;
	pthread_mutex_unlock( &b->lock );
	b->flag = negflag;
	return true;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .count = 0, .flag = 0, .lock = PTHREAD_MUTEX_INITIALIZER CBINIT() };
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierSenseRev Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
