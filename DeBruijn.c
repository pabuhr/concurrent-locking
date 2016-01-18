// Nicolaas Govert de Bruijn. Additional Comments on a Problem in Concurrent Programming Control, CACM, Mar 1967,
// 10(3):137. Letter to the Editor.

enum Intent { DontWantIn, WantIn, EnterCS };

static volatile TYPE *control CALIGN, turn CALIGN;

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST
	uint64_t entry;

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
		  L0: control[id] = WantIn;						// entry protocol
			Fence();									// force store before more loads
		  L1: for ( int j = turn; j != id; j = cycleDown( j, N ) )
				if ( control[j] != DontWantIn ) { Pause(); goto L1; } // restart search
			control[id] = EnterCS;
			Fence();									// force store before more loads
			for ( int j = N - 1; j >= 0; j -= 1 )
				if ( j != id && control[j] == EnterCS ) goto L0;
			CriticalSection( id );
			// cycle through threads
			if ( control[turn] == DontWantIn || turn == id ) // exit protocol
				turn = cycleDown( turn, N );
			control[id] = DontWantIn;
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
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
	control = Allocator( sizeof(typeof(control[0])) * N );
	for ( int i = 0; i < N; i += 1 ) {					// initialize shared data
		control[i] = DontWantIn;
	} // for
	turn = 0;
} // ctor

void dtor() {
	free( (void *)control );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=DeBruijn Harness.c -lpthread -lm" //
// End: //
