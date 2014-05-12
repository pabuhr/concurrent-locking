// G. L. Peterson, Myths About the Mutual Exclusion Problem, Information Processing Letters, 1981, 12(3), Fig. 1, p. 115
// Separate code for each thread is unified using an array.

enum Intent { DontWantIn, WantIn };
static volatile TYPE intents[2] = { DontWantIn, DontWantIn }, last;

static void *Worker( void *arg ) {
    const unsigned int id = (size_t)arg;
    uint64_t entry;

	int other = 1 - id;

    for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			intents[id] = WantIn;						// entry protocol
			last = id;									// RACE
			Fence();									// force store before more loads
			while ( intents[other] == WantIn && last == id ) Pause();
			CriticalSection( id );
			intents[id] = DontWantIn;					// exit protocol
			entry += 1;
		} // while
		entries[r][id] = entry;
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
    } // for
	return NULL;
} // Worker

void ctor() {
	assert( N == 2 );
} // ctor

void dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Peterson2 Harness.c -lpthread -lm" //
// End: //
