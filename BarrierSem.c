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
	}
}

static inline void block( Barrier * this ) {
	sem_wait( &this->mutex );
	this->count++;
	if ( this->count == this->size ) {
		sem_post_n( &this->turnstile );
	}
	sem_post( &this->mutex );

	sem_wait( &this->turnstile );
	
	sem_wait( &this->mutex );
	this->count--;
	if ( this->count == 0 ) {
		sem_post_n( &this->turnstile2 );
	}
	sem_post( &this->mutex );
	
	sem_wait( &this->turnstile2 );
} // block

//#define TESTING
#include "BarrierWorker.c"

const TYPE fanin = 2;

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b.count = 0;
	b.size = N;
	if ( sem_init(&b.mutex, 0, 1) != 0 )
		assert(false);
	if ( sem_init(&b.turnstile, 0, 0) != 0 )
		assert(false);
	if ( sem_init(&b.turnstile2, 0, 0) != 0 )
		assert(false);
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierSem Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
