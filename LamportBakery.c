// Leslie Lamport, A New Solution of Dijkstra's Concurrent Programming Problem, CACM, 1974, 17(8), p. 454

volatile TYPE *choosing;
volatile TYPE *ticket;

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			// step 1, select a ticket
			choosing[id] = 1;							// entry protocol
			Fence();									// force store before more loads
			TYPE max = 0;								// O(N) search for largest ticket
			for ( int j = 0; j < N; j += 1 ) {
				TYPE v = ticket[j];						// could change so copy
				if ( max < v ) max = v;
			} // for
#if 1
			max += 1;									// advance ticket
			ticket[id] = max;
			choosing[id] = 0;
			Fence();									// force store before more loads
			// step 2, wait for ticket to be selected
			for ( int j = 0; j < N; j += 1 ) {			// check other tickets
				while ( choosing[j] == 1 ) Pause();		// busy wait if thread selecting ticket
				while ( ticket[j] != 0 &&				// busy wait if choosing or
						( ticket[j] < max ||			//  greater ticket value or lower priority
						( ticket[j] == max && j < id ) ) ) Pause();
			} // for
#else
			ticket[id] = max + 1;						// advance ticket
			choosing[id] = 0;
			Fence();									// force store before more loads
			// step 2, wait for ticket to be selected
			for ( int j = 0; j < N; j += 1 ) {			// check other tickets
				while ( choosing[j] == 1 ) Pause();		// busy wait if thread selecting ticket
				while ( ticket[j] != 0 &&				// busy wait if choosing or
						( ticket[j] < ticket[id] ||		//  greater ticket value or lower priority
						( ticket[j] == ticket[id] && j < id ) ) ) Pause();
			} // for
#endif
			CriticalSection( id );
			ticket[id] = 0;								// exit protocol
			entry += 1;
		} // while
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

void ctor() {
	choosing = Allocator( sizeof(volatile TYPE) * N );
	ticket = Allocator( sizeof(volatile TYPE) * N );
	for ( int i = 0; i < N; i += 1 ) {					// initialize shared data
		choosing[i] = ticket[i] = 0;
	} // for
} // ctor

void dtor() {
	free( (void *)ticket );
	free( (void *)choosing );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=LamportBakery Harness.c -lpthread -lm" //
// End: //
