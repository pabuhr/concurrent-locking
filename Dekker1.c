// Edsger W. Dijkstra. Cooperating Sequential Processes. Technical Report,
// Technological University, Eindhoven, Netherlands 1965, pp. 58-59.

volatile TYPE c[2] = { 1, 1 }, turn = 1;

static void *Worker( void *arg ) {
    const unsigned int id = (size_t)arg;
    uint64_t entry;

	int other = 1 - id;

    for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
		  A1: c[id] = 0;
			Fence();
		  L1: if ( c[other] == 0 ) {
				if ( turn == id ) { Pause(); goto L1; }
				c[id] = 1;
				Fence();
			  B1: if ( turn == other ) { Pause(); goto B1; }
				goto A1;
			}
			CriticalSection( id );
			turn = other;
			c[id] = 1;
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
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Dekker1 Harness.c -lpthread -lm" //
// End: //
