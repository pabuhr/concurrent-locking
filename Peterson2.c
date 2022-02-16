// G. L. Peterson, Myths About the Mutual Exclusion Problem, Information Processing Letters, 1981, 12(3), Fig. 1, p. 115
// Separate code for each thread is unified using an array.

enum Intent { DontWantIn, WantIn };
static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE intents[2] CALIGN = { DontWantIn, DontWantIn }, last CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define inv( c ) ((c) ^ 1)

static void *Worker( void *arg ) {
    TYPE id = (size_t)arg;
    uint64_t entry;

	int other = inv( id );								// int is better than TYPE

#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

    for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			intents[id] = WantIn;						// entry protocol
			last = id;									// RACE
			Fence();									// force store before more loads
			while ( intents[other] != DontWantIn && last == id ) Pause(); // busy wait
			CriticalSection( id );
			intents[id] = DontWantIn;					// exit protocol
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			other = inv( id );
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			entry += 1;
		} // while
#ifdef FAST
		id = oid;
		other = inv( id );
#endif // FAST
		entries[r][id] = entry;
		Fai( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		Fai( &Arrived, -1 );
    } // for
	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	if ( N != 2 ) {
		printf( "\nUsage: N=%d must be 2\n", N );
		exit( EXIT_FAILURE);
	} // if
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Peterson2 Harness.c -lpthread -lm" //
// End: //
