// Edsger W. Dijkstra, Solution of a Problem in Concurrent Programming Control, CACM, 8(9), 1965, p. 569

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE * b CALIGN, * c CALIGN, turn CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg + 1;							// id 0 => don't-want-in
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
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
			for ( typeof(N) j = 1; j <= N; j += 1 )
				if ( j != (typeof(N))id && c[j] == 0 ) goto L;

			randomThreadChecksum += CriticalSection( id );

			b[id] = c[id] = 1;							// exit protocol
			turn = 0;

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		__sync_fetch_and_add( &sumOfThreadChecksums, randomThreadChecksum );

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

void __attribute__((noinline)) ctor() {
	b = Allocator( sizeof(typeof(b[0])) * (N + 1) );
	c = Allocator( sizeof(typeof(c[0])) * (N + 1) );
	for ( typeof(N) i = 0; i <= N; i += 1 ) {			// initialize shared data
		c[i] = b[i] = 1;
	} // for
	turn = 0;
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)c );
	free( (void *)b );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Dijkstra Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
