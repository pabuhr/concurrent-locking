// Boleslaw K. Szymanski. A simple solution to Lamport's concurrent programming problem with linear wait.
// Proceedings of the 2nd International Conference on Supercomputing, 1988, Figure 2, Page 624.
// Waiting after CS can be moved before it.

volatile TYPE *flag;

#define await( E ) while ( ! (E) ) Pause()

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
			flag[id] = 1;
			Fence();									// force store before more loads
			for ( j = 0; j < N; j += 1 )				// wait until doors open
				await( flag[j] < 3 );
			flag[id] = 3;								// close door 1
			Fence();									// force store before more loads
			for ( j = 0; j < N; j += 1 )				// check for 
				if ( flag[j] == 1 ) {					//   others in group ?
					flag[id] = 2;						// enter waiting room
					Fence();							// force store before more loads
				  L: for ( int k = 0; k < N; k += 1 )	// wait for
						if ( flag[k] == 4 ) goto fini;	//   door 2 to open
					goto L;
				  fini: ;
				} // if
			flag[id] = 4;								// open door 2
			Fence();									// force store before more loads
//			for ( j = 0; j < N; j += 1 )				// wait for all threads in waiting room
//				await( flag[j] < 2 || flag[j] > 3 );	//    to pass through door 2
			for ( j = 0; j < id; j += 1 )				// service threads in priority order
				await( flag[j] < 2 );
			CriticalSection( id );
			for ( j = id + 1; j < N; j += 1 )			// wait for all threads in waiting room
				await( flag[j] < 2 || flag[j] > 3 );	//    to pass through door 2
			flag[id] = 0;
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
} // ctor

void dtor() {
	free( (void *)flag );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=Szymanski Harness.c -lpthread -lm" //
// End: //
