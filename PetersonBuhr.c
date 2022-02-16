// Static and dynamic allocation of tables available.

#define inv( c ) ( ! (c) )

#include "Binary.c"

typedef struct CALIGN {
	Token *ns;											// pointer to path node from leaf to root
	TYPE es;											// left/right opponent
} Tuple;

static Tuple **states CALIGN;							// handle N threads
static int *levels CALIGN;								// minimal level for binary tree
//static Tuple states[64][6] CALIGN;						// handle 64 threads with maximal tree depth of 6 nodes (lg 64)
//static int levels[64] = { -1 } CALIGN;					// minimal level for binary tree
static Token *t CALIGN;
static TYPE PAD CALIGN __attribute__(( unused ));		// protect further false sharing

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	int level = levels[id];
	Tuple *state = states[id];

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			for ( int lv = 0; lv <= level; lv += 1 ) {		// entry protocol
				binary_prologue( state[lv].es, state[lv].ns );
			} // for

			CriticalSection( id );

			for ( int lv = level; lv >= 0; lv -= 1 ) {	// exit protocol, retract reverse order
				binary_epilogue( state[lv].es, state[lv].ns );
			} // for
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			entry += 1;
		} // while
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
	// element 0 not used
	t = Allocator( N * sizeof(typeof(t[0])) );

	// states[id][s].es indicates the left or right contender at a match.
	// states[id][s].ns is the address of the structure that contains the match data.
	// s ranges from 0 to the tree level of a start point (leaf) in a minimal binary tree.
	// levels[id] is level of start point minus 1 so bi-directional tree traversal is uniform.

	states = Allocator( N * sizeof(typeof(states[0])) );
	levels = Allocator( N * sizeof(typeof(levels[0])) );
	levels[0] = -1;										// default for N=1
	for ( int id = 0; id < N; id += 1 ) {
		t[id].Q[0] = t[id].Q[1] = 0;
#if defined( KESSELS2 )
		t[id].R[0] = t[id].R[1] = 0;
#else
		t[id].R = 0;
#endif // KESSELS2
		unsigned int start = N + id, level = Log2( start );
		states[id] = Allocator( level * sizeof(typeof(states[0][0])) );
		levels[id] = level - 1;
		for ( unsigned int s = 0; start > 1; start >>= 1, s += 1 ) {
			states[id][s].es = start & 1;
			states[id][s].ns = &t[start >> 1];
		} // for
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)levels );
	free( (void *)states );
	free( (void *)t );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=PetersonBuhr Harness.c -lpthread -lm" //
// End: //
