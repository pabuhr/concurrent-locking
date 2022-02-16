// Leslie Lamport, A New Solution of Dijkstra's Concurrent Programming Problem, CACM, 1974, 17(8), p. 454

#include <stdbool.h>

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE * choosing CALIGN, * ticket CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	typeof(&choosing[0]) mychoosing = &choosing[id];	// optimization
	typeof(&ticket[0]) myticket = &ticket[id];

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			// step 1, select a ticket
			*mychoosing = true;							// entry protocol
			Fence();									// force store before more loads
			TYPE max = 0;
			for ( typeof(N) j = 0; j < N; j += 1 ) {	// O(N) search for largest ticket
				TYPE v = ticket[j];						// could change so must copy
				if ( max < v ) max = v;
			} // for

			max += 1;									// advance ticket
			*myticket = max;
			WO( Fence(); );
			*mychoosing = false;						// finished ticket selection
			Fence();									// force store before more loads

			// step 2, wait for ticket to be selected
			for ( typeof(N) j = 0; j < N; j += 1 ) {	// check other tickets
				typeof(&choosing[0]) otherchoosing = &choosing[j]; // optimization
				while ( *otherchoosing ) Pause();		// busy wait if thread selecting ticket
				WO( Fence(); );
				typeof(&ticket[0]) otherticket = &ticket[j]; // optimization
				while ( *otherticket != 0 &&			// busy wait if choosing or
						( *otherticket < max ||			//  greater ticket value or lower priority
						( *otherticket == max && j < id ) ) ) Pause();
			} // for
			WO( Fence(); );

			randomThreadChecksum += CriticalSection( id );

			WO( Fence(); );
			*myticket = 0;								// exit protocol
			WO( Fence(); );

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			mychoosing = &choosing[id] ;				// optimization
			myticket = &ticket[id];
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
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=LamportBakery Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
