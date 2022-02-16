// Nancy A. Lynch, Distributed Algorithms, Morgan Kaufmann, 1996, Section 10.5.3
// Significant parts of this algorithm are written in prose, and therefore, left to our interpretation with respect to implementation.

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE * intents CALIGN, * turns CALIGN;
static TYPE depth CALIGN, width CALIGN, mask CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static inline TYPE min( TYPE a, TYPE b ) { return a < b ? a : b; }

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	TYPE lid, comp, role, low, high;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			for ( TYPE km1 = 0, k = 1; k <= depth; km1 += 1, k += 1 ) { // entry protocol
				lid = id >> km1;						// local id
				comp = (lid >> 1) + (width >> k);		// unique position in the tree
				role = lid & 1;							// left or right descendent
				intents[id] = k;						// declare intent, current round
				turns[comp] = role;						// RACE
				Fence();								// force store before more loads
				low = ((lid) ^ 1) << km1;				// lower competition
				high = min( low | mask >> (depth - km1), N - 1 ); // higher competition
				for ( TYPE i = low; i <= high; i += 1 )	// busy wait
					while ( intents[i] >= k && turns[comp] == role ) Pause();
			} // for
			WO( Fence(); );

			randomThreadChecksum += CriticalSection( id );

			WO( Fence(); );
			intents[id] = 0;							// exit protocol

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		Fai( &sumOfThreadChecksums, randomThreadChecksum );

		#ifdef FAST
		id = oid;
		#endif // FAST
		entries[r][id] = entry;
		Fai( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		Fai( &Arrived, -1 );
	} // for

	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	depth = Clog2( N );									// maximal depth of binary tree
	width = 1 << depth;									// maximal width of binary tree
	mask = width - 1;									// 1 bits for masking
	intents = Allocator( sizeof(typeof(intents[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {					// initialize shared data
		intents[i] = 0;
	} // for
	turns = Allocator( sizeof(typeof(turns[0])) * width );
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)turns );
	free( (void *)intents );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Lynch Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
