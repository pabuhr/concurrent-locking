// Eric C. R. Hehner and R. K. Shyamasundar, An Implementation of P and V, Information Processing Letters, 1981, 12(4),
// pp. 196-197

enum { MAX_TICKET = INTPTR_MAX };

volatile TYPE *ticket;

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
	int max, v;
	int j;
#ifdef FAST
	unsigned int cnt = 0;
#endif // FAST
	size_t entries[RUNS];

	for ( int r = 0; r < RUNS; r += 1 ) {
		entries[r] = 0;
		while ( stop == 0 ) {
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			// step 1, select a ticket
			ticket[id] = 0;								// set highest priority
			Fence();									// force store before more loads
			max = 0;									// O(N) search for largest ticket
			for ( j = 0; j < N; j += 1 ) {
				v = ticket[j];							// could change so copy
				if ( v != MAX_TICKET && max < v ) max = v;
			} // for
#if 1
			max += 1;									// advance ticket
			ticket[id] = max;
			Fence();									// force store before more loads
			// step 2, wait for ticket to be selected
			for ( j = 0; j < N; j += 1 )				// check other tickets
				while ( ticket[j] < max ||				// busy wait if choosing or
						( ticket[j] == max && j < id ) ) Pause(); //  greater ticket value or lower priority
#else
			ticket[id] = max + 1;
			Fence();									// force store before more loads
			// step 2, wait for ticket to be selected
			for ( j = 0; j < N; j += 1 )				// check other tickets
				while ( ticket[j] < ticket[id] ||		// busy wait if choosing or
						( ticket[j] == ticket[id] && j < id ) ) Pause(); //  greater ticket value or lower priority
#endif
			CriticalSection( id );
			ticket[id] = MAX_TICKET;					// exit protocol
			entries[r] += 1;
		} // while
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	qsort( entries, RUNS, sizeof(size_t), compare );
	return (void *)median(entries);
} // Worker

void ctor() {
	ticket = Allocator( sizeof(volatile TYPE) * N );
	for ( int i = 0; i < N; i += 1 ) {					// initialize shared data
		ticket[i] = MAX_TICKET;
	} // for
} // ctor

void dtor() {
	free( (void *)ticket );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=Hehner Harness.c -lpthread -lm" //
// End: //
