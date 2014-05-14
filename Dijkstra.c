// Edsger W. Dijkstra, Solution of a Problem in Concurrent Programming Control, CACM, 8(9), 1965, p. 569

volatile TYPE *b, *c, turn;

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg + 1;					// id 0 => don't-want-in
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			b[id] = 0;									// entry protocol
		  L: c[id] = 1;
			Fence();									// force store before more loads
			if ( turn != id ) {							// maybe set and restarted
				while ( b[turn] != 1 ) Pause();			// busy wait
				turn = id;
				Fence();								// force store before more loads
			} // if
			c[id] = 0;
			Fence();									// force store before more loads
			for ( int j = 1; j <= N; j += 1 )
				if ( j != id && c[j] == 0 ) goto L;
			CriticalSection( id );
			b[id] = c[id] = 1;							// exit protocol
			turn = 0;
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			entry += 1;
		} // while
#ifdef FAST
		id = oid;
#endif // FAST
		entries[r][id - 1] = entry;						// adjust for id + 1
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	return NULL;
} // Worker

void ctor() {
	b = Allocator( sizeof(volatile TYPE) * (N + 1) );
	c = Allocator( sizeof(volatile TYPE) * (N + 1) );
	for ( int i = 0; i <= N; i += 1 ) {					// initialize shared data
		c[i] = b[i] = 1;
	} // for
	turn = 0;
} // ctor

void dtor() {
	free( (void *)c );
	free( (void *)b );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Dijkstra Harness.c -lpthread -lm" //
// End: //
