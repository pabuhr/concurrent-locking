// Gary L. Peterson and Michael J. Fischer, Economical Solutions for the Critical Section Problem in a Distributed
// System (Extended Abstract), Proceedings of the Ninth Annual ACM Symposium on Theory of Computing, STOC '77, 1977, p. 93

#include <stdint.h>										// uint*

typedef union {
	uint32_t atom;										// ensure atomic assignment
	struct {
		uint16_t level;									// current level in tournament
		uint16_t state;									// intent to enter critical section
	} tuple;
} Tuple;

#define L(t) ((t).tuple.level)
#define R(s) ((s).tuple.state)
#define EQ(a, b) ((a).tuple.state == (b).tuple.state)
int bit( int i, int k ) { return (i & (1 << (k - 1))) != 0; }
int min( int a, int b ) { return a < b ? a : b; }

int depth, mask;
volatile Tuple *Q;

uint32_t QMAX( int w, unsigned int id, int k ) {
	int low = ((id >> (k - 1)) ^ 1) << (k - 1);
	int high = min( low | mask >> (depth - (k - 1)), N );
	Tuple opp;
	for ( int i = low; i <= high; i += 1 ) {
		opp.atom = Q[i].atom;
		if ( L(opp) >= k ) return opp.atom;
	} // for
	return (Tuple){ .tuple = {0, 0} }.atom;
} // QMAX

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
	Tuple opp;
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
			for ( int k = 1; k <= depth; k += 1 ) {		// entry protocol, round
				opp.atom = QMAX( 1, id, k );
				Fence();								// force store before more loads
				Q[id].atom = L(opp) == k ? (Tuple){ .tuple = {k, bit(id,k) ^ R(opp)} }.atom : (Tuple){ .tuple = {k, 1} }.atom;
				Fence();								// force store before more loads
				opp.atom = QMAX( 2, id, k );
				Fence();								// force store before more loads
				Q[id].atom = L(opp) == k ? (Tuple){ .tuple = {k, bit(id,k) ^ R(opp)} }.atom : Q[id].atom;
//			  wait:	opp.atom = QMAX( id, k );
//				Fence();								// force store before more loads
//				if ( (L(opp) == k && (bit(id,k) ^ EQ(opp, Q[id]))) || L(opp) > k ) { Pause(); goto wait; }
//				int cnt = 0, max = -1;
				Fence();								// force store before more loads
				while ( L(opp) > k || (L(opp) == k && (bit(id,k) ^ EQ(opp, Q[id]))) ) {
					Pause();
					opp.atom = QMAX( 3, id, k );
				} // while
			} // for
			CriticalSection( id );
			Q[id].atom = (Tuple){ .tuple = {0, 0} }.atom; // exit protocol
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
	int width = 1 << depth;								// maximal width of binary tree
	mask = width - 1;									// 1 bits for masking
	Q = Allocator( sizeof(volatile Tuple) * width );
	for ( int i = 0; i < width; i += 1 ) {				// initialize shared data
		Q[i].atom = (Tuple){ .tuple = {0, 0} }.atom;
	} // for
} // ctor

void dtor() {
	free( (void *)Q );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=PetersonT Harness.c -lpthread -lm" //
// End: //
