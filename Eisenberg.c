// Murray A. Eisenberg and Michael R. McGuire}, Further Comments on Dijkstra's Concurrent Programming Control Problem,
// CACM, 1972, 15(11), p. 999

enum Intent { DontWantIn, WantIn, EnterCS };

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE * control CALIGN, HIGH CALIGN;
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
			// step 1, wait for threads with higher priority
		  L1: for ( typeof(id) j = HIGH; j != id; j = cycleUp( j, N ) )
				if ( control[j] != DontWantIn ) { Pause(); goto L1; } // restart search
			control[id] = EnterCS;
			Fence();									// force store before more loads
			// step 2, check for any other thread finished step 1
			for ( typeof(N) j = 0; j < N; j += 1 )
				if ( j != id && control[j] == EnterCS ) goto L0;
			if ( control[HIGH] != DontWantIn && HIGH != id ) goto L0;
			HIGH = id;									// its now ok to enter

			randomThreadChecksum += CriticalSection( id );

			// look for any thread that wants in other than this thread
//			for ( typeof(N) j = cycleUp( id + 1, N );; j = cycleUp( j, N ) ) // exit protocol
			for ( typeof(N) j = cycleUp( HIGH + 1, N );; j = cycleUp( j, N ) ) // exit protocol
				if ( control[j] != DontWantIn ) { HIGH = j; break; }
			control[id] = DontWantIn;

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
	control = Allocator( sizeof(typeof(control[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {			// initialize shared data
		control[i] = DontWantIn;
	} // for
	HIGH = 0;
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)control );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Eisenberg Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
