// Dave Dice and Alex Kogan, Hemlock: Compact and Scalable Mutual Exclusion, Proceedings of the 33rd ACM Symposium on
// Parallelism in Algorithms and Architectures, July 2021, 173-183, Listing 1.

#include <stdbool.h>

typedef VTYPE Grant;
#ifndef ATOMIC
typedef Grant * volatile HemLock;
#else
typedef _Atomic(Grant *) HemLock;
#endif // ! ATOMIC

#define THREADLOCAL /* fastest version */

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static HemLock lock CALIGN;
#ifdef THREADLOCAL
static __thread volatile Grant grant CALIGN;
#define GPARM
#define GARG
#define STAR
#define ADDR &
#else
#define GPARM , Grant * grant
#define GARG , &grant
#define STAR *
#define ADDR
#endif // THREADLOCAL
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static inline void hem_lock( HemLock * lock  GPARM ) {
	STAR grant = false;
	Grant * prev = Fas( lock, ADDR grant );
	if ( prev != NULL ) { 
		await( Fas( prev, false ) );
	} // if
	WO( Fence(); );
} // hem_lock

static inline void hem_unlock( HemLock * lock  GPARM ) {
	WO( Fence(); );
	STAR grant = true;
	if ( ! Cas( lock, ADDR grant, NULL ) ) {
		await( ! STAR grant );
	} // if
} // hem_unlock

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	#ifndef THREADLOCAL
	Grant grant CALIGN;
	#endif // ! THREADLOCAL

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			hem_lock( &lock  GARG );

			randomThreadChecksum += CS( id );

			hem_unlock( &lock  GARG );

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		Fai( &sumOfThreadChecksums, randomThreadChecksum );

		#ifdef FAST
		id = oid;
		#endif // FAST
		entries[r][id] = entry;
		Fai( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		Fai( &Arrived, -1 );
	} // for

	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	lock = NULL;
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=HemLock Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
