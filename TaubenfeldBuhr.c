#define inv( c ) ((c) ^ 1)

#include "Binary.c"

static volatile Token **t CALIGN;

unsigned int depth CALIGN;

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	unsigned int lid;									// local id at each tree level

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			lid = id;									// entry protocol
			for ( int lv = 0; lv < depth; lv += 1 ) {
				binary_prologue( lid & 1, &t[lv][lid >> 1] );
				lid >>= 1;								// advance local id for next tree level
			} // for

			CriticalSection( id );

			for ( int lv = depth - 1; lv >= 0; lv -= 1 ) { // exit protocol, retract reverse order
				lid = id >> lv;
				binary_epilogue( lid & 1, &t[lv][lid >> 1] );
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
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	return NULL;
} // Worker

void ctor() {
	depth = Clog2( N );									// maximal depth of binary tree
	int width = 1 << depth;								// maximal width of binary tree
	t = Allocator( sizeof(typeof(t[0])) * depth );		// allocate matrix columns
	for ( int r = 0; r < depth; r += 1 ) {				// allocate matrix rows
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

void dtor() {
	for ( int r = 0; r < depth; r += 1 ) {				// deallocate matrix rows
		free( (void *)t[r] );
	} // for
	free( (void *)t );									// deallocate matrix columns
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=TaubenfeldBuhr Harness.c -lpthread -lm" //
// End: //
