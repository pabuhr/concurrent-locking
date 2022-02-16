// Leslie Lamport, A Fast Mutual Exclusion Algorithm, ACM Transactions on Computer Systems, 5(1), 1987, Fig. 2, p. 5
// N => do not want in, versus 0 in original paper, so "b" is dimensioned 0..N-1 rather than 1..N.

#include <stdbool.h>

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE * b CALIGN;
static VTYPE x CALIGN, y CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static TYPE Bottom = UINTPTR_MAX;

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
		  START: ;
			b[id] = true;								// entry protocol
			WO( Fence(); )								// write order matters
			x = id;
			Fence();									// write/read order matters
			if ( SLOWPATH( y != Bottom ) ) {
				b[id] = false;
				Fence();								// write/read order matters
				await( y == Bottom );
				goto START;
			} // if
			y = id;
			Fence();									// write/read order matters
			if ( SLOWPATH( x != id ) ) {
				b[id] = false;
				Fence();								// write/read order matters
				for ( typeof(N) k = 0; k < N; k += 1 )
					await( ! b[k] );
				// If the loop consistently reads an outdated value of y (== id from assignment above), there is only
				// the danger of starvation, and that is unlikely. Correctness only requires the value read after the
				// loop is recent.
				WO( Fence(); )							// read recent y
				if ( SLOWPATH( y != id ) ) {
					await( y == Bottom );				// optional
					goto START;
				} // if
			} // if
			WO( Fence(); )

			randomThreadChecksum += CriticalSection( id );

			WO( Fence(); );								// prevent write floating up
			y = Bottom;									// exit protocol
			WO( Fence(); );								// write order matters
			b[id] = false;
			WO( Fence(); );

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
	b = Allocator( sizeof(typeof(b[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {			// initialize shared data
		b[i] = false;
	} // for
	y = Bottom;
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)b );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=LamportFast Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
