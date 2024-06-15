// Allen B Downey, The Little Book of Semaphores, Green Tea Press, 2016, p. 41, Listing 3.10

#include <semaphore.h>

typedef	struct CALIGN SemBarrier {
	TYPE count, size;
	sem_t mutex, turnstile, turnstile2;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline void sem_post_n( sem_t * sem ) { 
	for ( TYPE i = 0; i < N; i++ ) {
		sem_post( sem );
	} // for
}

static inline void block( Barrier * b ) {
	sem_wait( &b->mutex );
	b->count++;
	if ( b->count == b->size ) {
		sem_post_n( &b->turnstile );
	}
	sem_post( &b->mutex );

	sem_wait( &b->turnstile );
	
	sem_wait( &b->mutex );
	b->count--;
	if ( b->count == 0 ) {
		sem_post_n( &b->turnstile2 );
	}
	sem_post( &b->mutex );
	
	sem_wait( &b->turnstile2 );
} // block

#include "BarrierWorker.c"

const TYPE fanin = 2;

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b.count = 0;
	b.size = N;
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
