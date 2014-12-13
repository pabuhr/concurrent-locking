// Yih-Kuen Tsay, Deriving a Scalable Algorithm for Mutual Exclusion,
// Distributed Computing, Lecture Notes in Computer Science Volume 1499, 1998, pp 393-407 

enum Intent { DontWantIn, WantIn };
static volatile TYPE intents[2] = { DontWantIn, DontWantIn }, last;

static void *Worker( void *arg ) {
    TYPE id = (size_t)arg;
    uint64_t entry;

	int other = 1 - id;									// int is better than TYPE

#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

    for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			intents[id] = WantIn;						// entry protocol
			last = id;									// RACE
			Fence();									// force store before more loads
			if ( intents[other] != DontWantIn )			// local spin
				while ( last == id ) Pause();			// busy wait
			CriticalSection( id );
			intents[id] = DontWantIn;					// exit protocol
			last = id;

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
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Tsay Harness.c -lpthread -lm" //
// End: //
