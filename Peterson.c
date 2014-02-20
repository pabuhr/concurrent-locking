// G. L. Peterson, Myths About the Mutual Exclusion Problem, Information Processing Letters, 1981, 12(3), Fig. 3, p. 116
// cnt is used to prove threads do not move evenly through levels.

volatile TYPE *Q, *turns;

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg + 1;					// id 0 => don't-want-in
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

//	unsigned long int cnt[N];

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
//		for ( int i = 1; i < N; i += 1 ) cnt[i] = 0;
		while ( stop == 0 ) {
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			for ( TYPE rd = 1; rd < N; rd += 1 ) {		// entry protocol, round
				Q[id] = rd;								// current round
				turns[rd] = id;							// RACE
				Fence();								// force store before more loads
			  L: for ( int k = 1; k <= N; k += 1 ) {		// find loser
//					if ( k != id && Q[k] == rd ) cnt[rd] += 1;
					if ( k != id && Q[k] >= rd && turns[rd] == id ) { Pause(); goto L; }
				} // for
			} // for
			CriticalSection( id );
			Q[id] = 0;									// exit protocol
			entry += 1;
		} // while
#ifdef FAST
		id = oid;
#endif // FAST
		entries[r][id - 1] = entry;						// adjust for id + 1
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
//		printf( "%d: %ld %ld %ld %ld %ld %ld %ld\n",
//				id, cnt[1]/entries[r][id], cnt[2]/entries[r][id], cnt[3]/entries[r][id], cnt[4]/entries[r][id], cnt[5]/entries[r][id], cnt[6]/entries[r][id], cnt[7]/entries[r][id] ); // make print atomic
	} // for
	return NULL;
} // Worker

void ctor() {
	Q = Allocator( sizeof(volatile TYPE) * (N + 1) );
	turns = Allocator( sizeof(volatile TYPE) * (N - 1 + 1) );
	for ( int i = 1; i <= N; i += 1 ) {					// initialize shared data
		Q[i] = 0;
	} // for
} // ctor

void dtor() {
	free( (void *)turns );
	free( (void *)Q );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=Peterson Harness.c -lpthread -lm" //
// End: //
