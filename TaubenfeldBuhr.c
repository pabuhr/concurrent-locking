#include xstr(FCFS.h)									// include algorithm for testing

#define inv( c ) ((c) ^ 1)

#include "Binary.c"

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
#ifndef ATOMIC
static volatile Token ** t;								// triangular matrix, Token already CALIGN
#else
_Atomic(volatile Token **) t;
#endif // ! ATOMIC
TYPE depth CALIGN;
FCFSGlobal();
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	FCFSLocal();

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	TYPE lid;											// local id at each tree level
	TYPE mydepth = depth;								// local copy of depth

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			FCFSEnter();
			lid = id;									// entry protocol
			for ( typeof(mydepth) lv = 0; lv < mydepth; lv += 1 ) {
				binary_prologue( lid & 1, &t[lv][lid >> 1] );
				lid >>= 1;								// advance local id for next tree level
			} // for
			FCFSExitAcq();
			WO( Fence(); );

			randomThreadChecksum += CS( id );

			FCFSTestExit();
			WO( Fence(); );								// prevent write floating up
			for ( int lv = mydepth - 1; lv >= 0; lv -= 1 ) { // exit protocol, retract reverse order
				lid = id >> lv;
				binary_epilogue( lid & 1, &t[lv][lid >> 1] );
			} // for
			FCFSExitRel();

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
	int width = 1 << depth;								// maximal width of binary tree
	t = Allocator( sizeof(typeof(t[0])) * depth );		// allocate matrix columns
	for ( typeof(depth) r = 0; r < depth; r += 1 ) {	// allocate matrix rows
		int size = width >> r;							// maximal row size
		t[r] = Allocator( sizeof(typeof(t[0][0])) * size );
		for ( int c = 0; c < size; c += 1 ) {			// initial all intents to dont-want-in
			t[r][c].Q[0] = t[r][c].Q[1] = 0;
			#if defined( KESSELS2 )
			t[r][c].R[0] = t[r][c].R[1] = 0;
			#else
			t[r][c].R = 0;
			#endif // KESSELS2
		} // for
	} // for
	FCFSCtor();
} // ctor

void __attribute__((noinline)) dtor() {
	FCFSDtor();
	for ( typeof(depth) r = 0; r < depth; r += 1 ) {	// deallocate matrix rows
		free( (void *)t[r] );
	} // for
	free( (void *)t );									// deallocate matrix columns
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=TaubenfeldBuhr Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
