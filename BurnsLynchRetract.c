// James E. Burns and Nancy A. Lynch, Mutual Exclusion using Indivisible Reads and Writes, Proceedings of the 18th
// Annual Allerton Conference on Communications, Control and Computing, 1980, p. 836

enum Intent { DontWantIn, WantIn };

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE * intents CALIGN;							// shared
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	typeof(N) j;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
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
			Fence();									// force store before more loads

			randomThreadChecksum += CriticalSection( id );

			Fence();									// force store before more loads
			intents[id] = DontWantIn;					// exit protocol

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
	intents = Allocator( sizeof(typeof(intents[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {			// initialize shared data
		intents[i] = DontWantIn;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)intents );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BurnsLynchRetract Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
