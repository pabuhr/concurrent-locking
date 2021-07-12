// Eric C. R. Hehner and R. K. Shyamasundar, An Implementation of P and V, Information Processing Letters, 1981, 12(4),
// pp. 196-197

enum { MAX_TICKET = INTPTR_MAX };

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE * ticket CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	VTYPE * myticket = &ticket[id];						// optimization

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			// step 1, select a ticket
			*myticket = 0;								// set highest priority
			Fence();									// force store before more loads
			TYPE max = 0;
			for ( typeof(N) j = 0; j < N; j += 1 ) {	// O(N) search for largest ticket
				TYPE v = ticket[j];						// could change so must copy
				if ( max < v && v != MAX_TICKET ) max = v;
			} // for

			max += 1;									// advance ticket
			*myticket = max;
			Fence();									// force store before more loads

			// step 2, wait for ticket to be selected
			for ( typeof(N) j = 0; j < N; j += 1 ) {	// check other tickets
				VTYPE * otherticket = &ticket[j];		// optimization
				while ( *otherticket < max ||			// busy wait if choosing or
						( *otherticket == max && j < id ) ) Pause(); //  greater ticket value or lower priority
			} // for
			WO( Fence(); );

			randomThreadChecksum += CriticalSection( id );

			WO( Fence(); );
			*myticket = MAX_TICKET;						// exit protocol

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			myticket = &ticket[id];						// optimization
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
	ticket = Allocator( sizeof(typeof(ticket[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {			// initialize shared data
		ticket[i] = MAX_TICKET;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)ticket );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Hehner Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
