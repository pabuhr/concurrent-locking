// Boleslaw K. Szymanski. A simple solution to Lamport's concurrent programming problem with linear wait.
// Proceedings of the 2nd International Conference on Supercomputing, 1988, Figure 2, Page 624.
// Waiting after CS can be moved before it.

volatile TYPE *flag;

#define await( E ) while ( ! (E) ) Pause()

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	int j;

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
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
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Szymanski Harness.c -lpthread -lm" //
// End: //
