// Edsger W. Dijkstra. Cooperating Sequential Processes. Technical Report,
// Technological University, Eindhoven, Netherlands 1965, pp. 58-59.

enum Intent { DontWantIn, WantIn };
static volatile TYPE cc[2] CALIGN = { DontWantIn, DontWantIn }, turn CALIGN = 0;

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
		  A1: cc[id] = WantIn;
			Fence();
		  L1: if ( cc[other] == WantIn ) {
				if ( turn == id ) { /* Pause(); */ goto L1; }
				cc[id] = DontWantIn;
				Fence();
			  B1: if ( turn == other ) { Pause(); goto B1; }
				goto A1;
			}
			CriticalSection( id );
			turn = other;
			cc[id] = DontWantIn;

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
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=DekkerOrig Harness.c -lpthread -lm" //
// End: //
