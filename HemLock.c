// Dave Dice and Alex Kogan, Hemlock: Compact and Scalable Mutual Exclusion, Proceedings of the 33rd ACM Symposium on
// Parallelism in Algorithms and Architectures, July 2021, 173-183.

#include <stdbool.h>

typedef struct { VTYPE Grant; } Element;
typedef struct { Element * volatile Tail; } HemLock;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static HemLock L CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			Element E CALIGN = { 0 };
			Element * const prev = Fas( &L.Tail, &E );
			if ( prev != NULL ) { 
				await( Fas( &(prev->Grant), 0 ) != 0 );
			} // if
			WO( Fence(); );

			randomThreadChecksum += CriticalSection( id );

			WO( Fence(); );
			E.Grant = 1;
			if ( ! Cas( &L.Tail, &E, NULL ) ) {
				#ifdef __ARM_ARCH
				await( E.Grant == 0 );
				#else
				await( Fas( &(E.Grant), 1 ) == 0 );
				#endif // __ARM_ARCH
			} // if

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
	L.Tail = NULL;
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=HemLock Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
