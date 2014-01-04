// Nancy A. Lynch, Distributed Algorithms, Morgan Kaufmann, 1996, Section 10.5.3
// Significant parts of this algorithm are written in prose, and therefore, left to our interpretation with respect to implementation.

static volatile TYPE *intents, *turns;
static int depth, width, mask;

static inline TYPE min( TYPE a, TYPE b ) { return a < b ? a : b; }

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
	unsigned int lid, comp, role, low, high;
#ifdef FAST
	unsigned int cnt = 0;
#endif // FAST
	size_t entries[RUNS];

	for ( int r = 0; r < RUNS; r += 1 ) {
		entries[r] = 0;
		while ( stop == 0 ) {
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			for ( TYPE km1 = 0, k = 1; k <= depth; km1 += 1, k += 1 ) { // entry protocol
				lid = id >> km1;						// local id
				comp = (lid >> 1) + (width >> k);		// unique position in the tree
				role = lid & 1;							// left or right descendent
				intents[id] = k;						// declare intent, current round
				turns[comp] = role;						// RACE
				Fence();								// force store before more loads
				low = ((lid) ^ 1) << km1;				// lower competition
				high = min( low | mask >> (depth - km1), N - 1 ); // higher competition
				for ( int i = low; i <= high; i += 1 )	// busy wait
					while ( intents[i] >= k && turns[comp] == role ) Pause();
			} // for
			CriticalSection( id );
			intents[id] = 0;							// exit protocol
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
	depth = Clog2( N );									// maximal depth of binary tree
	width = 1 << depth;									// maximal width of binary tree
	mask = width - 1;									// 1 bits for masking
	intents = Allocator( sizeof(volatile TYPE) * N );
	for ( int i = 0; i < N; i += 1 ) {					// initialize shared data
		intents[i] = 0;
	} // for
	turns = Allocator( sizeof(volatile TYPE) * width );
} // ctor

void dtor() {
	free( (void *)turns );
	free( (void *)intents );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=Lynch Harness.c -lpthread -lm" //
// End: //
