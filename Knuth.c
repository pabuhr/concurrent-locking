// Donald E. Knuth, Additional Comments on a Problem in Concurrent Programming Control, CACM, 1966, 9(5), p. 321,
// Letter to the Editor

enum Intent { DontWantIn, WantIn, EnterCS };

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE * control CALIGN, turn CALIGN;
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
		  L0: control[id] = WantIn;						// entry protocol
			Fence();									// force store before more loads
		  L1: for ( typeof(id) j = turn; j != id; j = cycleDown( j, N ) )
				if ( control[j] != DontWantIn ) { Pause(); goto L1; } // restart search
			control[id] = EnterCS;
			Fence();									// force store before more loads
			for ( int j = N - 1; j >= 0; j -= 1 )
				if ( j != (int)id && control[j] == EnterCS ) goto L0;
//			turn = id;

			randomThreadChecksum += CriticalSection( id );

			// cycle through threads
			turn = cycleDown( id, N );					// exit protocol
			control[id] = DontWantIn;

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
	control = Allocator( sizeof(typeof(control[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {					// initialize shared data
		control[i] = DontWantIn;
	} // for
	turn = 0;
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)control );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Knuth Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
