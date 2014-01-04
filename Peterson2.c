// G. L. Peterson, Myths About the Mutual Exclusion Problem, Information Processing Letters, 1981, 12(3), Fig. 1, p. 115
// Separate code for each thread is unified using an array.

enum Intent { DontWantIn, WantIn };
static volatile TYPE intents[2] = { DontWantIn, DontWantIn }, last;

static void *Worker( void *arg ) {
    const unsigned int id = (size_t)arg;
	int other = 1 - id;
    size_t entries[RUNS];

    for ( int r = 0; r < RUNS; r += 1 ) {
		entries[r] = 0;
		while ( stop == 0 ) {
			intents[id] = WantIn;						// entry protocol
			last = id;									// RACE
			Fence();
			while ( intents[other] == WantIn && last == id ) Pause();
			CriticalSection( id );
			intents[id] = DontWantIn;					// exit protocol
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
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=Peterson2 Harness.c -lpthread -lm" //
// End: //

