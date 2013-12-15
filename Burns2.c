// James E. Burns, Symmetry in Systems of Asynchronous Processes. 22nd Annual
// Symposium on Foundations of Computer Science, 1981, Figure 2, p 170.

volatile TYPE *flag, turn;

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
		  L0: flag[id] = 1;								// entry protocol
			turn = id;									// RACE
			Fence();									// force store before more loads
		  L1: if ( turn != id ) {
				flag[id] = 0;
				Fence();								// force store before more loads
			  L11: for ( j = 0; j < N; j += 1 )
					if ( j != id && flag[j] != 0 ) { Pause(); goto L11; }
				goto L0;
			} else {
//				flag[id] = 1;
//				Fence();								// force store before more loads
			  L2: if ( turn != id ) goto L1;
				for ( j = 0; j < N; j += 1 )
					if ( j != id && flag[j] != 0 ) goto L2;
			} // if
			CriticalSection( id );
			flag[id] = 0;								// exit protocol
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
	flag = Allocator( sizeof(volatile TYPE) * N );
	for ( int i = 0; i < N; i += 1 ) {					// initialize shared data
		flag[i] = 0;
	} // for
	//turn = 0;
} // ctor

void dtor() {
	free( (void *)flag );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=Burns2 Harness.c -lpthread -lm" //
// End: //
