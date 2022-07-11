// James H. Anderson and Yong-Jik Kim, A New Fast-Path Mechanism For Mutual Exclusion, Distributed Computing, 14(10),
// 2001, Fig. 4, p. 22

#include <stdbool.h>

#define inv( c ) ( (c) ^ 1 )

#include "Binary.c"

#ifdef TB

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE ** intents CALIGN;							// triangular matrix of intents
static VTYPE ** turns CALIGN;							// triangular matrix of turns
static unsigned int depth CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#else

typedef struct CALIGN {
	Token * ns;											// pointer to path node from leaf to root
	TYPE es;											// left/right opponent
} Tuple;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Tuple ** states CALIGN;							// handle N threads
static int * levels CALIGN;								// minimal level for binary tree
//static Tuple states[64][6] CALIGN;						// handle 64 threads with maximal tree depth of 6 nodes (lg 64)
//static int levels[64] = { -1 } CALIGN;					// minimal level for binary tree
static Token * t CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#endif // TB

//======================================================

static inline void entrySlow(
	#ifdef TB
	TYPE id
	#else
	int level, Tuple * state
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
	int level, Tuple * state
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

//======================================================

typedef union {
	struct {
		VHALFSIZE free;
		VHALFSIZE indx;
	};
	WHOLESIZE atom;										// ensure atomic assignment
	VWHOLESIZE vatom;									// volatile alias
} Atomic;

static TYPE PAD3 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE x CALIGN;
static Atomic y CALIGN, Reset CALIGN;					// volatile fields
static VTYPE * Name_Taken, * Obstacle;
static VTYPE Infast CALIGN;
static Token B; // = { { 0, 0 }, 0 };
static TYPE PAD4 CALIGN __attribute__(( unused ));		// protect further false sharing

static inline void SLOW1( TYPE id, uint32_t * randomThreadChecksum
						  #ifndef TB
						  , int level, Tuple * state
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

	*randomThreadChecksum += CS( id );

	binary_epilogue( 0, &B );
	exitSlow(
		#ifdef TB
		id
		#else
		level, state
		#endif // TB
		);
} // SLOW1


static inline void SLOW2( TYPE id, Atomic y, uint32_t * randomThreadChecksum
						  #ifndef TB
						  , int level, Tuple * state
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

	*randomThreadChecksum += CS( id );

	y.vatom = 0;
	x = id;
	Fence();											// force store before more loads
	y.vatom = Reset.vatom;
	Obstacle[id] = false;
	Reset.vatom = (Atomic){ { false, y.indx } }.atom;
	Fence();											// force store before more loads
	if ( ! Name_Taken[ y.indx ] && ! Obstacle[ y.indx ] ) {
		HALFSIZE temp = (HALFSIZE)(y.indx + 1 < N ? y.indx + 1 : 0);
		Reset.vatom = (Atomic){ { true, temp } }.atom;
		y.vatom = (Atomic){ { true, temp } }.atom;
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


static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	#ifndef TB
	int level = levels[id];
	Tuple * state = states[id];
	#endif // ! TB

	Atomic ly;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			x = id;
			Fence();									// force store before more loads
			ly.atom = y.vatom;
			if ( FASTPATH( ! ly.free ) ) {
				SLOW1( id, &randomThreadChecksum
					   #ifndef TB
					   , level, state
					   #endif // ! TB
					);
			} else {
				y.vatom = 0;
				Obstacle[id] = true;
				Fence();								// force store before more loads
				if ( FASTPATH( x != id || Infast ) ) {
					SLOW2( id, ly, &randomThreadChecksum
						   #ifndef TB
						   , level, state
						   #endif // ! TB
						);
				} else {
					Name_Taken[ly.indx] = true;
					Fence();							// force store before more loads
					if ( FASTPATH( Reset.vatom != ly.atom ) ) {
						Name_Taken[ly.indx] = false;
						SLOW2( id, ly, &randomThreadChecksum
							   #ifndef TB
							   , level, state
							   #endif // ! TB
							);
					} else {
						Infast = true;
						binary_prologue( 1, &B );

						randomThreadChecksum += CS( id );

						Obstacle[id] = false;
						Reset.vatom = (Atomic){ { false, ly.indx } }.atom;
						Fence();						// force store before more loads
						// Obstacle[ ly.indx ] is always false for minimal contentions, and almost always false for
						// maximal contention => LIKELY for both minimal and maximal contention.
						if ( LIKELY( ! Obstacle[ ly.indx ] ) ) { // always
							HALFSIZE temp = (HALFSIZE)(ly.indx + 1 < N ? ly.indx + 1 : 0);
							Reset.vatom = (Atomic){ { true, temp } }.atom;
							y.vatom = (Atomic){ { true, temp } }.atom;
						} // if
						Name_Taken[ly.indx] = false;
						binary_epilogue( 1, &B );
						Infast = false;
					} // if
				} // if
			} // if

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
	for ( typeof(N) id = 0; id < N; id += 1 ) {
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
	for ( typeof(N) i = 0; i < N; i += 1 ) {			// initialize shared data
		Name_Taken[i] = Obstacle[i] = false;
	} // for
	y.vatom = (Atomic){ { true, 0 } }.atom;
	Reset.vatom = (Atomic){ { true, 0 } }.atom;
	Infast = false;
	ctor2();											// tournament allocation/initialization
} // ctor

void __attribute__((noinline)) dtor2() {
	#ifdef TB
	for ( typeof(depth) r = 0; r < depth; r += 1 ) {	// deallocate matrix rows
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
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=AndersonKim Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
