#ifdef TB

static volatile TYPE **intents;							// triangular matrix of intents
static volatile TYPE **turns;							// triangular matrix of turns
static unsigned int depth;

#else

typedef struct {
	TYPE Q[2], R, PAD;									// make structure even multiple of word size by padding
} Token;

typedef struct {
	TYPE es;											// left/right opponent
	volatile Token *ns;									// pointer to path node from leaf to root
} Tuple;

static Tuple **states;									// handle 64 threads with maximal tree depth of 6 nodes (lg 64)
static int *levels;										// minimal level for binary tree
//static Tuple states[64][6];								// handle 64 threads with maximal tree depth of 6 nodes (lg 64)
//static int levels[64] = { -1 };							// minimal level for binary tree

static volatile Token *t;

#define inv( c ) (1 - c)

static inline void binary_prologue( TYPE c, volatile Token *t ) {
	t->Q[c] = 1;
	t->R = c;											// RACE
	Fence();											// force store before more loads
	while ( t->Q[inv( c )] && t->R == c ) Pause();		// busy wait
} // binary_prologue

static inline void binary_epilogue( TYPE c, volatile Token *t ) {
	t->Q[c] = 0;
} // binary_epilogue

#endif // TB

//======================================================

#include <stdbool.h>

static volatile TYPE *b;
static volatile TYPE x, y, bintents[2] = { false, false }, last;

#define await( E ) while ( ! (E) ) Pause()

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

#ifdef TB
	unsigned int ridt, ridi;
#else
	int level = levels[id];
	Tuple *state = states[id];
#endif // TB

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
#if 0
			if ( FASTPATH( y == N ) ) {
				b[id] = true;
				x = id;
				Fence();								// force store before more loads
				if ( FASTPATH( y == N ) ) {
					y = id;
					Fence();							// force store before more loads
					if ( FASTPATH( x == id ) ) {
						goto cont;
					} else {
						b[id] = false;
						Fence();						// force store before more loads
						for ( int j = 0; j < N; j += 1 )
							await( y != id || ! b[j] );
						if ( FASTPATH( y == id ) )
							goto cont;
					} // if
				} else {
					b[id] = false;
				} // if
			} // if
			goto aside;
		  cont: ;
#else
			if ( FASTPATH( y != N ) ) goto aside;
			b[id] = true;								// entry protocol
			x = id;
			Fence();									// force store before more loads
			if ( FASTPATH( y != N ) ) {
				b[id] = false;
				goto aside;
			} // if
			y = id;
			Fence();									// force store before more loads
			if ( FASTPATH( x != id ) ) {
				b[id] = false;
				Fence();								// force store before more loads
				for ( int j = 0; j < N; j += 1 )
					await( y != id || ! b[j] );
				if ( FASTPATH( y != id ) ) goto aside;
			} // if
#endif
			bintents[0] = true;
#ifndef DEKKER
			last = false;
#endif // ! DEKKER
			Fence();									// force store before more loads
#ifndef DEKKER
			await( ! bintents[1] || last );
#else
			while ( bintents[1] ) {
				if ( ! last ) {
					bintents[0] = false;
					Fence();							// force store before more loads
					while ( ! last ) Pause();			// low priority busy wait
					bintents[0] = true;
					Fence();							// force store before more loads
				} else {
					Pause();
				} // if
			} // while
#endif // ! DEKKER

			CriticalSection( id );

#ifdef DEKKER
			last = false;
#endif // DEKKER
			bintents[0] = false;
			y = N;
			b[id] = false;
			goto fini;

		  aside:
#ifdef TB
#if defined( __sparc )
			__asm__ volatile( "" : : : "memory" );
#endif // __sparc
//			ridi = id;
			for ( unsigned int lv = 0; lv < depth; lv += 1 ) {	// entry protocol
				ridi = id >> lv;						// round id for intent
				ridt = ridi >> 1;						// round id for turn
// Comment out the next 5 lines to improve performance on x86 for FAST
				intents[lv][ridi] = 1;					// declare intent
				turns[lv][ridt] = ridi;					// RACE
				Fence();								// force store before more loads
				while ( intents[lv][ridi ^ 1] == 1 && turns[lv][ridt] == ridi ) Pause();
//				ridi = ridi >> 1;
			} // for
#else
#if defined( __sparc )
			__asm__ volatile( "" : : : "memory" );
#endif // __sparc
			for ( int s = 0; s <= level; s += 1 ) { // entry protocol
				binary_prologue( state[s].es, state[s].ns );
			} // for
#endif // TB

			bintents[1] = true;
#ifndef DEKKER
			last = true;
#endif // ! DEKKER
			Fence();									// force store before more loads
#ifndef DEKKER
			await( ! bintents[0] || ! last );
#else
			while ( bintents[0] ) {
				if ( last ) {
					bintents[1] = false;
					Fence();							// force store before more loads
					while ( last ) Pause();				// low priority busy wait
					bintents[1] = true;
					Fence();							// force store before more loads
				} else {
					Pause();
				} // if
			} // while
#endif // ! DEKKER

			CriticalSection( id );

#ifdef DEKKER
			last = true;
#endif // DEKKER
			bintents[1] = false;

#ifdef TB
			for ( int lv = depth - 1; lv >= 0; lv -= 1 ) { // exit protocol
				intents[lv][id >> lv] = 0;				// retract all intents in reverse order
			} // for
#else
			for ( int s = level; s >= 0; s -= 1 ) {		// exit protocol, reverse order
				binary_epilogue( state[s].es, state[s].ns );
			} // for
#endif // TB
		  fini:
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
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	return NULL;
} // Worker

void __attribute__((noinline)) ctor2() {
#ifdef TB
	depth = Clog2( N );									// maximal depth of binary tree
	int width = 1 << depth;								// maximal width of binary tree
	intents = Allocator( sizeof(volatile TYPE *) * depth );	// allocate matrix columns
	turns = Allocator( sizeof(volatile TYPE *) * depth );
	for ( unsigned int r = 0; r < depth; r += 1 ) {		// allocate matrix rows
		unsigned int size = width >> r;					// maximal row size
		intents[r] = Allocator( sizeof(TYPE) * size );
		for ( unsigned int c = 0; c < size; c += 1 ) {	// initial all intents to dont-want-in
			intents[r][c] = 0;
		} // for
		turns[r] = Allocator( sizeof(TYPE) * (size >> 1) );	// half maximal row size
	} // for
#else
	// element 0 not used
	t = Allocator( sizeof(volatile Token) * N );

	// states[id][s].es indicates the left or right contender at a match.
	// states[id][s].ns is the address of the structure that contains the match data.
	// s ranges from 0 to the tree level of a start point (leaf) in a minimal binary tree.
	// levels[id] is level of start point minus 1 so bi-directional tree traversal is uniform.

	states = Allocator( sizeof(Tuple *) * N );
	levels = Allocator( sizeof(int) * N );
	levels[0] = -1;										// default for N=1
	for ( unsigned int id = 0; id < N; id += 1 ) {
		t[id].Q[0] = t[id].Q[1] = 0;
		unsigned int start = N + id, level = Log2( start );
		states[id] = Allocator( sizeof(Tuple) * level );
		levels[id] = level - 1;
		for ( unsigned int s = 0; start > 1; start >>= 1, s += 1 ) {
			states[id][s].es = start & 1;
			states[id][s].ns = &t[start >> 1];
		} // for
	} // for
#endif // TB
} // ctor2

void __attribute__((noinline)) ctor() {
	b = Allocator( sizeof(volatile TYPE) * N );
	for ( int i = 0; i < N; i += 1 ) {					// initialize shared data
		b[i] = 0;
	} // for
	y = N;
	ctor2();											// tournament allocation/initialization
} // ctor

void __attribute__((noinline)) dtor2() {
#ifdef TB
	for ( int r = 0; r < depth; r += 1 ) {				// deallocate matrix rows
		free( (void *)turns[r] );
		free( (void *)intents[r] );
	} // for
	free( (void *)turns );								// deallocate matrix columns
	free( (void *)intents );
#else
	free( (void *)levels );
	free( (void *)states );
	free( (void *)t );
#endif // TB
} // dtor2

void __attribute__((noinline)) dtor() {
	free( (void *)b );
	dtor2();											// tournament deallocation
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Triangle Harness.c -lpthread -lm" //
// End: //
