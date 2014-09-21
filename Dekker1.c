// Edsger W. Dijkstra. Cooperating Sequential Processes. Technical Report,
// Technological University, Eindhoven, Netherlands 1965, pp. 58-59.

static volatile TYPE cc[2] CALIGN = { 0, 0 }, turn CALIGN = 0;

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST
	int other = 1 - id;

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
		  A1: cc[id] = 1;
			Fence();
		  L1: if ( cc[other] == 1 ) {
				if ( turn == id ) { Pause(); goto L1; }
				cc[id] = 0;
				Fence();
			  B1: if ( turn == other ) { Pause(); goto B1; }
				goto A1;
			}
			CriticalSection( id );
			turn = other;
			cc[id] = 0;
			entry += 1;
		} // while
		entries[r][id] = entry;
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	assert( N == 2 );
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Dekker1 Harness.c -lpthread -lm" //
// End: //
