#define inv( c ) ((c) ^ 1)

#include "Binary.c"

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static volatile Token ** t CALIGN;
unsigned int depth CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	unsigned int lid;									// local id at each tree level

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			lid = id;									// entry protocol
			for ( typeof(depth) lv = 0; lv < depth; lv += 1 ) {
				binary_prologue( lid & 1, &t[lv][lid >> 1] );
				lid >>= 1;								// advance local id for next tree level
			} // for

			randomThreadChecksum += CriticalSection( id );

			for ( int lv = depth - 1; lv >= 0; lv -= 1 ) { // exit protocol, retract reverse order
				lid = id >> lv;
				binary_epilogue( lid & 1, &t[lv][lid >> 1] );
			} // for

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		__sync_fetch_and_add( &sumOfThreadChecksums, randomThreadChecksum );

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

void __attribute__((noinline)) ctor() {
	depth = Clog2( N );									// maximal depth of binary tree
	int width = 1 << depth;								// maximal width of binary tree
	t = Allocator( sizeof(typeof(t[0])) * depth );		// allocate matrix columns
	for ( typeof(depth) r = 0; r < depth; r += 1 ) {				// allocate matrix rows
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
} // ctor

void __attribute__((noinline)) dtor() {
	for ( typeof(depth) r = 0; r < depth; r += 1 ) {	// deallocate matrix rows
		free( (void *)t[r] );
	} // for
	free( (void *)t );									// deallocate matrix columns
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=TaubenfeldBuhr Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
