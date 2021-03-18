// Leslie Lamport, A New Solution of Dijkstra's Concurrent Programming Problem, CACM, 1974, 17(8), p. 454

#include <stdbool.h>
#include <stdatomic.h>

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
volatile _Atomic TYPE * choosing CALIGN, * ticket CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		uint32_t randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			// step 1, select a ticket
			choosing[id] = true;						// entry protocol
			TYPE max = 0;
			for ( typeof(N) j = 0; j < N; j += 1 ) {	// O(N) search for largest ticket
				TYPE v = ticket[j];						// could change so must copy
				if ( max < v ) max = v;
			} // for

			max += 1;									// advance ticket
			ticket[id] = max;
			choosing[id] = false;						// finished ticket selection

			// step 2, wait for ticket to be selected
			for ( typeof(N) j = 0; j < N; j += 1 ) {	// check other tickets
				while ( choosing[j] ) Pause();			// busy wait if thread selecting ticket
				while ( ticket[j] != 0 &&				// busy wait if choosing or
						( ticket[j] < max ||			//  greater ticket value or lower priority
						( j < id && ticket[j] == max ) ) ) Pause();
			} // for

			randomThreadChecksum += CriticalSection( id );

			ticket[id] = 0;								// exit protocol

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		__sync_fetch_and_add( &sumOfThreadChecksums, randomThreadChecksum );

		#ifdef FAST
		id = oid;
		#endif // FAST
		entries[r][id] = entry;
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for

	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	choosing = Allocator( sizeof(typeof(choosing[0])) * N );
	ticket = Allocator( sizeof(typeof(ticket[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {			// initialize shared data
		choosing[i] = ticket[i] = 0;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)ticket );
	free( (void *)choosing );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=LamportBakeryA Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
