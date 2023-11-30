// https://en.wikipedia.org/wiki/Barrier_(computer_science)

typedef struct CALIGN {
	TYPE arrive_counter;								// processes in the barrier
	VTYPE leave_counter;								// processes exited the barrier
	VTYPE flag;
	pthread_mutex_t lock;
} barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b, p );

void block( barrier * b, TYPE p ) {
	pthread_mutex_lock( &b->lock );
	if ( b->arrive_counter == 0 ) {
		if ( b->leave_counter == p ) {					// no other threads in barrier
			b->flag = 0;								// first arriver clears flag
		} else {
			pthread_mutex_unlock( &b->lock );
			while ( b->leave_counter != p ) Pause();	// wait for all to leave before clearing
			pthread_mutex_lock( &b->lock );
			b->flag = 0;
		} // if
	} // if
	++(b->arrive_counter);
	pthread_mutex_unlock( &b->lock );
	if ( b->arrive_counter == p ) {						// last arriver sets flag
		b->arrive_counter = 0;
		b->leave_counter = 1;
		b->flag = 1;
	} else {
		while ( b->flag == 0 );							// wait for flag
		pthread_mutex_lock( &b->lock );
		b->leave_counter++;
		pthread_mutex_unlock( &b->lock );
	} // if
} // block

#define TESTING
#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (barrier){ .arrive_counter = 0, .leave_counter = N, .flag= 0, .lock = PTHREAD_MUTEX_INITIALIZER };
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierLock Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
