// James E. Burns and Nancy A. Lynch, {Mutual Exclusion using Indivisible Reads and Writes, Proceedings of the 18th
// Annual Allerton Conference on Communications, Control and Computing, 1980, p. 836

enum Intent { DontWantIn, WantIn };
volatile TYPE *intents;									// shared

static void *Worker( void *arg ) {
	const unsigned int id = (size_t)arg;
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
		  L0: intents[id] = DontWantIn;					// entry protocol
			Fence();									// force store before more loads
			for ( j = 0; j < id; j += 1 )
				if ( intents[j] == WantIn ) { Pause(); goto L0; }
			intents[id] = WantIn;
			Fence();									// force store before more loads
			for ( j = 0; j < id; j += 1 )
				if ( intents[j] == WantIn ) goto L0;
		  L1: for ( j = id + 1; j < N; j += 1 )
				if ( intents[j] == WantIn ) { Pause(); goto L1; }
			CriticalSection( id );						// critical section
			intents[id] = DontWantIn;					// exit protocol
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
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=BurnsLynchRetractIntent Harness.c -lpthread -lm" //
// End: //
