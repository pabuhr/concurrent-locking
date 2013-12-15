// Peter A. Buhr and Ashif S. Harji, Concurrent Urban Legends, Concurrency and Computation: Practice and Experience,
// 2005, 17(9), Figure 3, p. 1151

enum Intent { DontWantIn, WantIn };
volatile TYPE intents[2] = { DontWantIn, DontWantIn }, last = 0;

static void *Worker( void *arg ) {
	const unsigned int id = (size_t)arg;
	int other = 1 - id;
	size_t entries[RUNS];

	for ( int r = 0; r < RUNS; r += 1 ) {
		entries[r] = 0;
		while ( stop == 0 ) {
			intents[id] = WantIn;
			// Necessary to prevent the read of intents[other] from floating above the assignment
			// intents[id] = WantIn, when the hardware determines the two subscripts are different.
			Fence();
			// Non-atomic because equality check and only single assignment to intents.
			while ( intents[other] == WantIn ) {
				if ( last == id ) {
					intents[id] = DontWantIn;
					// Optional fence to prevent LD of "last" from being lifted above store of
					// intends[id]=DontWantIn. Because a thread only writes its own id into "last",
					// and because of eventual consistency (writes eventually become visible),
					// the fence is conservative.
					Fence();
					while ( last == id ) Pause();		// low priority busy wait
				} else {
					Pause();
				} // if
				intents[id] = WantIn;
				Fence();
			} // while
			CriticalSection( id );
			last = id;									// exit protocol
			intents[id] = DontWantIn;
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
	assert( N == 2 );
} // ctor

void dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=Dekker2 Harness.c -lpthread -lm" //
// End: //
