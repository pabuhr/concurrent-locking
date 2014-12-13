// Peter A. Buhr and Ashif S. Harji, Concurrent Urban Legends, Concurrency and Computation: Practice and Experience,
// 2005, 17(9), Figure 3, p. 1151

enum Intent { DontWantIn, WantIn };
static volatile TYPE intents[2] CALIGN = { DontWantIn, DontWantIn }, last CALIGN = 0;

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	int other = id ^ 1;									// int is better than TYPE

#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
#ifdef FLICKER
			for ( int i = 0; i < 100; i += 1 ) intents[id] = i % 2; // flicker
#endif // FLICKER
			intents[id] = WantIn;
			// Necessary to prevent the read of intents[other] from floating above the assignment
			// intents[id] = WantIn, when the hardware determines the two subscripts are different.
			Fence();									// force store before more loads
			while ( intents[other] == WantIn ) {
				if ( last == id ) {
#ifdef FLICKER
					for ( int i = 0; i < 100; i += 1 ) intents[id] = i % 2; // flicker
#endif // FLICKER
					intents[id] = DontWantIn;
					// Optional fence to prevent LD of "last" from being lifted above store of
					// intends[id]=DontWantIn. Because a thread only writes its own id into "last",
					// and because of eventual consistency (writes eventually become visible),
					// the fence is conservative.
					Fence();							// force store before more loads
					while ( last == id ) Pause();		// low priority busy wait
#ifdef FLICKER
					for ( int i = 1; i < 100; i += 1 ) intents[id] = i % 2; // flicker
#endif // FLICKER
					intents[id] = WantIn;
					Fence();							// force store before more loads
//				} else {
//					Pause();
				} // if
			} // while
			CriticalSection( id );
#ifdef FLICKER
			for ( int i = id; i < 100; i += 1 ) last = i % 2; // flicker
#endif // FLICKER
			last = id;									// exit protocol
#ifdef FLICKER
			for ( int i = 0; i < 100; i += 1 ) intents[id] = i % 2; // flicker
#endif // FLICKER
			intents[id] = DontWantIn;

#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			other = 1 - id;
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			entry += 1;
		} // while
#ifdef FAST
		id = oid;
		other = 1 - id;
#endif // FAST
		entries[r][id] = entry;
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	if ( N < 1 || N > 2 ) {
		printf( "\nUsage: N=%d must be 1 or 2\n", N );
		exit( EXIT_FAILURE);
	} // if
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Dekker Harness.c -lpthread -lm" //
// End: //
