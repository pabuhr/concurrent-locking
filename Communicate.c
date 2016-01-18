static volatile TYPE turn CALIGN;

static void *Worker( void *arg ) {
	const TYPE id = (size_t)arg;
	uint64_t entry;

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			turn = id;									// perturb cache
			Fence();									// force store before more loads
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
	turn = 0;											// initialize shared data
} // ctor

void dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Communicate Harness.c -lpthread -lm" //
// End: //
