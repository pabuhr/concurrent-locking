// Donald E. Knuth, Additional Comments on a Problem in Concurrent Programming Control, CACM, 1966, 9(5), p. 321,
// Letter to the Editor

enum Intent { DontWantIn, WantIn, EnterCS };
volatile TYPE *control, turn;

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
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
		  L0: control[id] = WantIn;						// entry protocol
			Fence();									// force store before more loads
		  L1: for ( j = turn; j != id; j = cycleDown( j, N ) )
				if ( control[j] != DontWantIn ) { Pause(); goto L1; } // restart search
			control[id] = EnterCS;
			Fence();									// force store before more loads
			for ( j = N - 1; j >= 0; j -= 1 )
				if ( j != id && control[j] == EnterCS ) goto L0;
//			turn = id;
			CriticalSection( id );
			// cycle through threads
			turn = cycleDown( id, N );						// exit protocol
			control[id] = DontWantIn;
			entries[r] += 1;
#ifdef STRESSINTERVAL
			PollBarrier();
#endif // STRESSINTERVAL
		} // while
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	qsort( entries, RUNS, sizeof(size_t), compare );
	return (void *)median(entries);
} // Worker

void ctor() {
	control = Allocator( sizeof(volatile TYPE) * N );
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
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=Knuth Harness.c -lpthread -lm" //
// End: //
