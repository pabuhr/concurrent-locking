typedef struct {
	TYPE Q[2], R, PAD;									// make structure even multiple of word size by padding
} Token;
static volatile Token *t;

typedef struct {
	TYPE es;											// left/right opponent
	volatile Token *ns;									// pointer to path node from leaf to root
} Tuple;
static Tuple states[64][6];								// handle 64 threads with maximal tree depth of 6 nodes (lg 64)
static int levels[64] = { -1 };							// minimal level for binary tree

#define inv( c ) (1 - c)

static inline void binary_prologue( TYPE c, volatile Token *t ) {
#if 1		// Peterson
	t->Q[c] = 1;
	t->R = c;
	Fence();											// force store before more loads
	while ( t->Q[inv( c )] && t->R == c ) Pause();		// busy wait
#else		// Tsay
	t->Q[c] = 1;
	t->R = c;
	Fence();											// force store before more loads
	if ( t->Q[inv( c ) ) while ( t->R == c ) Pause();	// busy wait
#endif
} // binary_prologue

static inline void binary_epilogue( TYPE c, volatile Token *t ) {
#if 1		// Peterson
	t->Q[c] = 0;
#else		// Tsay
	t->Q[c] = 0;
	t->R = c;
#endif
} // binary_epilogue

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
	int s, level = levels[id];
	Tuple *state = states[id];
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
			for ( s = 0; s <= level; s += 1 ) {			// entry protocol
				binary_prologue( state[s].es, state[s].ns );
			} // for
			CriticalSection( id );
			for ( s = level; s >= 0; s -= 1 ) {			// exit protocol, reverse order
				binary_epilogue( state[s].es, state[s].ns );
			} // for
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
	// element 0 not used
	t = Allocator( sizeof(volatile Token) * N );

	// states[id][s].es indicates the left or right contender at a match.
	// states[id][s].ns is the address of the structure that contains the match data.
	// s ranges from 0 to the tree level of a start point (leaf) in a minimal binary tree.
	// levels[id] is level of start point minus 1 so bi-directional tree traversal is uniform.

	for ( unsigned int id = 0; id < N; id += 1 ) {
		t[id].Q[0] = t[id].Q[1] = 0;
		unsigned int start = N + id, level = Log2( start );
		levels[id] = level - 1;
		for ( unsigned int s = 0; start > 1; start >>= 1, s += 1 ) {
			states[id][s].es = start & 1;
			states[id][s].ns = &t[start >> 1];
		} // for
	} // for
} // ctor

void dtor() {
	free( (void *)t );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=PetersonBuhr Harness.c -lpthread -lm" //
// End: //
