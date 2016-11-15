// James H. Anderson and Yong-Jik Kim, A New Fast-Path Mechanism For Mutual Exclusion, Distributed Computing, 14(10),
// 2001, Fig. 4, p. 22

#include <stdbool.h>

#define inv( c ) ( (c) ^ 1 )

#include "Binary.c"

#ifdef TB

static volatile TYPE **intents CALIGN;					// triangular matrix of intents
static volatile TYPE **turns CALIGN;					// triangular matrix of turns
static unsigned int depth CALIGN;

#else

typedef struct CALIGN {
	Token *ns;											// pointer to path node from leaf to root
	TYPE es;											// left/right opponent
} Tuple;

static Tuple **states CALIGN;							// handle N threads
static int *levels CALIGN;								// minimal level for binary tree
//static Tuple states[64][6] CALIGN;						// handle 64 threads with maximal tree depth of 6 nodes (lg 64)
//static int levels[64] = { -1 } CALIGN;					// minimal level for binary tree
static Token *t CALIGN;

#endif // TB

//======================================================

static volatile TYPE x CALIGN, y CALIGN;
static volatile Token B; // = { { 0, 0 }, 0 };
static TYPE PAD CALIGN __attribute__(( unused ));		// protect further false sharing

#define await( E ) while ( ! (E) ) Pause()


static inline void entrySlow(
#ifdef TB
	TYPE id
#else
	int level, Tuple *state
#endif // TB
	) {
#ifdef TB
	unsigned int ridt, ridi;

//	ridi = id;
	for ( unsigned int lv = 0; lv < depth; lv += 1 ) {	// entry protocol
		ridi = id >> lv;								// round id for intent
		ridt = ridi >> 1;								// round id for turn
		intents[lv][ridi] = 1;							// declare intent
		turns[lv][ridt] = ridi;							// RACE
		Fence();										// force store before more loads
		while ( intents[lv][ridi ^ 1] == 1 && turns[lv][ridt] == ridi ) Pause();
//		ridi = ridi >> 1;
	} // for
#else
	for ( int s = 0; s <= level; s += 1 ) {				// entry protocol
		binary_prologue( state[s].es, state[s].ns );
	} // for
#endif // TB
} // entrySlow

static inline void exitSlow(
#ifdef TB
	TYPE id
#else
	int level, Tuple *state
#endif // TB
	) {
#ifdef TB
	for ( int lv = depth - 1; lv >= 0; lv -= 1 ) {		// exit protocol
		intents[lv][id >> lv] = 0;						// retract all intents in reverse order
	} // for
#else
	for ( int s = level; s >= 0; s -= 1 ) {				// exit protocol, reverse order
		binary_epilogue( state[s].es, state[s].ns );
	} // for
#endif // TB
} // exitSlow


//=========================================================================


typedef union {
	struct {
		uint16_t free;
		uint16_t indx;
	} tuple;
	uint32_t atom;										// can be 64, use 32 for SPARC
} Ytype;

static volatile TYPE X CALIGN;
static volatile Ytype Y CALIGN, Reset CALIGN;
static volatile TYPE *Name_Taken, *Obstacle;
static volatile TYPE Infast CALIGN;
static TYPE PAD CALIGN __attribute__(( unused ));		// protect further false sharing


static inline void SLOW1( TYPE id
#ifndef TB
						  , int level, Tuple *state
#endif // ! TB
	) {
	entrySlow(
#ifdef TB
		id
#else
		level, state
#endif // TB
		);
	binary_prologue( 0, &B );
	CriticalSection( id );
	binary_epilogue( 0, &B );
	exitSlow(
#ifdef TB
		id
#else
		level, state
#endif // TB
		);
} // SLOW1


static inline void SLOW2( TYPE id, Ytype y
#ifndef TB
						  , int level, Tuple *state
#endif // ! TB
	) {
	entrySlow(
#ifdef TB
		id
#else
		level, state
#endif // TB
		);
	binary_prologue( 0, &B );
	CriticalSection( id );
	Y.atom = 0;
	X = id;
	Fence();											// force store before more loads
	y.atom = Reset.atom;
	Obstacle[id] = 0;
	Reset.atom = (Ytype){ .tuple = { .free = 0, .indx = y.tuple.indx } }.atom;
	Fence();											// force store before more loads
	if ( ! Name_Taken[ y.tuple.indx ] && ! Obstacle[ y.tuple.indx ] ) {
		uint16_t temp = (uint16_t)(y.tuple.indx + 1 < N ? y.tuple.indx + 1 : 0);
		Reset.atom = (Ytype){ .tuple = { .free = 1, .indx = temp } }.atom;
		Y.atom = (Ytype){ .tuple = { .free = 1, .indx = temp } }.atom;
	} // if
	binary_epilogue( 0, &B );
	exitSlow(
#ifdef TB
		id
#else
		level, state
#endif // TB
		);
} // SLOW2


static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST
#ifndef TB
	int level = levels[id];
	Tuple *state = states[id];
#endif // ! TB

	Ytype y;

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {

			X = id;
			Fence();									// force store before more loads
			y.atom = Y.atom;
			if ( FASTPATH( ! y.tuple.free ) ) {
				SLOW1( id
#ifndef TB
					   , level, state
#endif // ! TB
					    );
			} else {
				Y.atom = 0;
				Obstacle[id] = 1;
				Fence();								// force store before more loads
				if ( FASTPATH( X != id || Infast ) ) {
					SLOW2( id, y
#ifndef TB
						   , level, state
#endif // ! TB
						);
				} else {
					Name_Taken[y.tuple.indx] = 1;
					Fence();							// force store before more loads
					if ( FASTPATH( Reset.atom != y.atom ) ) {
						Name_Taken[y.tuple.indx] = 0;
						SLOW2( id, y
#ifndef TB
							   , level, state
#endif // ! TB
							);
					} else {
						Infast = 1;
						binary_prologue( 1, &B );
						CriticalSection( id );
						Obstacle[id] = 0;
						Reset.atom = (Ytype){ .tuple = { .free = 0, .indx = y.tuple.indx } }.atom;
						Fence();						// force store before more loads
						if ( ! Obstacle[ y.tuple.indx ] ) {
							uint16_t temp = (uint16_t)(y.tuple.indx + 1 < N ? y.tuple.indx + 1 : 0);
							Reset.atom = (Ytype){ .tuple = { .free = 1, .indx = temp } }.atom;
							Y.atom = (Ytype){ .tuple = { .free = 1, .indx = temp } }.atom;
						} // if
						Name_Taken[y.tuple.indx] = 0;
						binary_epilogue( 1, &B );
						Infast = 0;
					} // if
				} // if
			} // if

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


//=========================================================================


void __attribute__((noinline)) ctor2() {
#ifdef TB
	depth = Clog2( N );									// maximal depth of binary tree
	int width = 1 << depth;								// maximal width of binary tree
	intents = Allocator( sizeof(typeof(intents[0])) * depth ); // allocate matrix columns
	turns = Allocator( sizeof(typeof(turns[0])) * depth );
	for ( unsigned int r = 0; r < depth; r += 1 ) {		// allocate matrix rows
		unsigned int size = width >> r;					// maximal row size
		intents[r] = Allocator( sizeof(typeof(intents[0][0])) * size );
		for ( unsigned int c = 0; c < size; c += 1 ) {	// initial all intents to dont-want-in
			intents[r][c] = 0;
		} // for
		turns[r] = Allocator( sizeof(typeof(turns[0][0])) * (size >> 1) ); // half maximal row size
	} // for
#else
	// element 0 not used
	t = Allocator( sizeof(typeof(t[0])) * N );

	// states[id][s].es indicates the left or right contender at a match.
	// states[id][s].ns is the address of the structure that contains the match data.
	// s ranges from 0 to the tree level of a start point (leaf) in a minimal binary tree.
	// levels[id] is level of start point minus 1 so bi-directional tree traversal is uniform.

	states = Allocator( sizeof(typeof(states[0])) * N );
	levels = Allocator( sizeof(typeof(levels[0])) * N );
	levels[0] = -1;										// default for N=1
	for ( TYPE id = 0; id < N; id += 1 ) {
		t[id].Q[0] = t[id].Q[1] = t[id].R = 0;
		unsigned int start = N + id, level = Log2( start );
		states[id] = Allocator( sizeof(typeof(states[0][0])) * level );
		levels[id] = level - 1;
		for ( unsigned int s = 0; start > 1; start >>= 1, s += 1 ) {
			states[id][s].es = start & 1;
			states[id][s].ns = &t[start >> 1];
		} // for
	} // for
#endif // TB
} // ctor2

void __attribute__((noinline)) ctor() {
	Name_Taken = Allocator( sizeof(typeof(Name_Taken[0])) * N );
	Obstacle = Allocator( sizeof(typeof(Obstacle[0])) * N );
	for ( int i = 0; i < N; i += 1 ) {					// initialize shared data
		Name_Taken[i] = Obstacle[i] = 0;
	} // for
	Y.atom = (Ytype){ .tuple = { .free = 1, .indx = 0 } }.atom;
	Reset.atom = (Ytype){ .tuple = { .free = 1, .indx = 0 } }.atom;
	Infast = 0;
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
	dtor2();											// tournament deallocation
	free( (void *)Obstacle );
	free( (void *)Name_Taken );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=AndersonKim Harness.c -lpthread -lm" //
// End: //
