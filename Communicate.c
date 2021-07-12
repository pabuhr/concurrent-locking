static VTYPE turn CALIGN;

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	for ( int r = 0; r < RUNS; r += 1 ) {
		for ( entry = 0; stop == 0; entry += 1 ) {
			turn = id;									// perturb cache
			Fence();									// force store before more loads
		} // for

		entries[r][id] = entry;
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	turn = 0;											// initialize shared data
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Communicate Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
