#include <pthread.h>

typedef pthread_barrier_t Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier barrier CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &barrier );

static inline void block( Barrier * b ) {
	if ( UNLIKELY( pthread_barrier_wait( b ) > 0 ) ) abort(); // ignore PTHREAD_BARRIER_SERIAL_THREAD & 0
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	if ( pthread_barrier_init( &barrier, NULL, N ) != 0 ) abort();
} // ctor

void __attribute__((noinline)) dtor() {
	if ( pthread_barrier_destroy( &barrier ) != 0 ) abort();
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierPthread Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
