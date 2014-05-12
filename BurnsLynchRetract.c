// James E. Burns and Nancy A. Lynch, {Mutual Exclusion using Indivisible Reads and Writes, Proceedings of the 18th
// Annual Allerton Conference on Communications, Control and Computing, 1980, p. 836

enum Intent { DontWantIn, WantIn };
volatile TYPE *intents;									// shared

static void *Worker( void *arg ) {
	const unsigned int id = (size_t)arg;
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
		  L0: intents[id] = DontWantIn;					// entry protocol
			Fence();									// force store before more loads
			for ( int j = 0; j < id; j += 1 )
				if ( intents[j] == WantIn ) { Pause(); goto L0; }
			intents[id] = WantIn;
			Fence();									// force store before more loads
			for ( int j = 0; j < id; j += 1 )
				if ( intents[j] == WantIn ) goto L0;
		  L1: for ( j = id + 1; j < N; j += 1 )
				if ( intents[j] == WantIn ) { Pause(); goto L1; }
			CriticalSection( id );						// critical section
			intents[id] = DontWantIn;					// exit protocol
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
	intents = Allocator( sizeof(volatile TYPE) * N );
	for ( int i = 0; i < N; i += 1 ) {					// initialize shared data
		intents[i] = DontWantIn;
	} // for
} // ctor

void dtor() {
	free( (void *)intents );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BurnsLynchRetractIntent Harness.c -lpthread -lm" //
// End: //
