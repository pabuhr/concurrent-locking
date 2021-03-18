// Edward A. Lycklama and Vassos Hadzilacos, A First-Come-First-Served Mutual-Exclusion Algorithm with Small
// Communication Variables, TOPLAS, 13(4), 1991, Fig. 4, p. 569
// Replaced pairs of bits by the values 0, 1, 2, respectively, and cycle through these values using modulo arithmetic.

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static volatile TYPE * c CALIGN, * v CALIGN, * intents CALIGN, * turn CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	TYPE copy[N];
	typeof(N) j;

	for ( int r = 0; r < RUNS; r += 1 ) {
		uint32_t randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			c[id] = 1;									// stage 1, establish FCFS
			Fence();									// force store before more loads
			for ( j = 0; j < N; j += 1 )				// copy turn values
				copy[j] = turn[j];
//			turn[id] = cycleUp( turn[id], 4 );			// advance my turn
			turn[id] = cycleUp( turn[id], 3 );			// advance my turn
			v[id] = 1;
			c[id] = 0;
			Fence();									// force store before more loads
			for ( j = 0; j < N; j += 1 )
				while ( c[j] != 0 || (v[j] != 0 && copy[j] == turn[j]) ) Pause();
		  L: intents[id] = 1;							// B-L
			Fence();									// force store before more loads
			for ( j = 0; j < id; j += 1 )				// stage 2, high priority search
				if ( intents[j] != 0 ) {
					intents[id] = 0;
					Fence();							// force store before more loads
					while ( intents[j] != 0 ) Pause();
					goto L;
				} // if
			for ( j = id + 1; j < N; j += 1 )			// stage 3, low priority search
				while ( intents[j] != 0 ) Pause();
			WO( Fence(); );

			randomThreadChecksum += CriticalSection( id );

			WO( Fence(); );
			v[id] = intents[id] = 0;					// exit protocol
			WO( Fence(); );

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
	c = Allocator( sizeof(typeof(c[0])) * N );
	v = Allocator( sizeof(typeof(v[0])) * N );
	intents = Allocator( sizeof(typeof(intents[0])) * N );
	turn = Allocator( sizeof(typeof(turn[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		c[i] = v[i] = intents[i] = turn[i] = 0;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)turn );
	free( (void *)intents );
	free( (void *)v );
	free( (void *)c );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=LycklamaBuhr Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
