// Gadi Taubenfeld, Synchronization Algorithms and Concurrent Programming, Pearson/Prentice Hall, 2006, p. 38

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE ** intents CALIGN;							// triangular matrix of intents
static VTYPE ** turns CALIGN;							// triangular matrix of turns
static unsigned int depth CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			unsigned int node = id;
			for ( typeof(depth) lv = 0; lv < depth; lv += 1 ) {	// entry protocol
				unsigned int lr = node & 1;				// round id for intent
				node >>= 1;								// round id for turn
				intents[lv][2 * node + lr] = 1;			// declare intent
				turns[lv][node] = lr;					// RACE
				Fence();								// force store before more loads
				while ( intents[lv][2 * node + (1 - lr)] == 1 && turns[lv][node] == lr ) Pause();
			} // for

			randomThreadChecksum += CS( id );

			for ( int lv = depth - 1; lv >= 0; lv -= 1 ) { // exit protocol
				intents[lv][id / (1 << lv)] = 0;		// retract all intents in reverse order
			} // for

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		Fai( sumOfThreadChecksums, randomThreadChecksum );

		#ifdef FAST
		id = oid;
		#endif // FAST
		entries[r][id] = entry;
		Fai( Arrived, 1 );
		while ( stop != 0 ) Pause();
		Fai( Arrived, -1 );
	} // for
	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	depth = Clog2( N );									// maximal depth of binary tree
	int width = 1 << depth;								// maximal width of binary tree
	intents = Allocator( sizeof(typeof(intents[0])) * depth ); // allocate matrix columns
	turns = Allocator( sizeof(typeof(turns[0])) * depth );
	for ( typeof(depth) r = 0; r < depth; r += 1 ) {	// allocate matrix rows
		int size = width >> r;							// maximal row size
		intents[r] = Allocator( sizeof(typeof(intents[0][0])) * size );
		for ( int c = 0; c < size; c += 1 ) {			// initial all intents to dont-want-in
			intents[r][c] = 0;
		} // for
		//printf( "depth %d width %d size %d size >> 1 %d\n", depth, width, size, size >> 1 );
		turns[r] = Allocator( sizeof(typeof(turns[0][0])) * (size >> 1) ); // half maximal row size
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	for ( typeof(depth) r = 0; r < depth; r += 1 ) {	// deallocate matrix rows
		free( (void *)turns[r] );
		free( (void *)intents[r] );
	} // for
	free( (void *)turns );								// deallocate matrix columns
	free( (void *)intents );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Taubenfeld Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
