// Allen B Downey, The Little Book of Semaphores, Green Tea Press, 2016, p. 41, Listing 3.10
// Possibly the worst barrier.

#include "BarrierCallback.h"

#include <semaphore.h>

typedef	struct CALIGN SemBarrier {
	TYPE CALIGN group;
	TYPE CALIGN count;
	sem_t mutex, turnstile, turnstile2;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline void sem_post_n( TYPE group, sem_t * sem ) { 
	for ( TYPE i = 0; i < group; i += 1 ) {
		sem_post( sem );
	} // for
} // sem_post_n

static inline bool block( Barrier * b ) {
	CBSTART();											// must be first
	bool ret;
	sem_wait( &b->mutex );
	b->count += 1;
	if ( b->count == b->group ) {
		sem_post_n( b->group, &b->turnstile );
	} // if

	sem_post( &b->mutex );
	sem_wait( &b->turnstile );
	sem_wait( &b->mutex );

	b->count -= 1;
	if ( UNLIKELY( b->count == 0 ) ) {
		CBEND();										// must appear in safe location
		sem_post_n( b->group, &b->turnstile2 );
		ret = true;
	} else ret = false;

	sem_post( &b->mutex );
	sem_wait( &b->turnstile2 );
	return ret;
} // block

#include "BarrierWorker.c"

const TYPE fanin = 2;

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .count = 0 CBINIT() };
	if ( sem_init( &b.mutex, 0, 1 ) != 0 ) abort();
	if ( sem_init( &b.turnstile, 0, 0 ) != 0 ) abort();
	if ( sem_init( &b.turnstile2, 0, 0 ) != 0 ) abort();
} // ctor

void __attribute__((noinline)) dtor() {
	if ( sem_destroy( &b.mutex ) != 0 ) abort();
	if ( sem_destroy( &b.turnstile ) != 0 ) abort();
	if ( sem_destroy( &b.turnstile2 ) != 0 ) abort();
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierSem Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
