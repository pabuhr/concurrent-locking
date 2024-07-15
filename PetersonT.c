// Gary L. Peterson and Michael J. Fischer, Economical Solutions for the Critical Section Problem in a Distributed
// System (Extended Abstract), Proceedings of the Ninth Annual ACM Symposium on Theory of Computing, STOC '77, 1977, p. 93

#include <stdint.h>										// uint*_t

typedef union {
	struct {
		VHALFSIZE level;								// current level in tournament
		VHALFSIZE state;								// intent to enter critical section
	};
	WHOLESIZE atom;										// ensure atomic assignment
	VWHOLESIZE vatom;									// volatile alias
} Tuple;

#define L(t) ((t).level)
#define R(s) ((s).state)
#define EQ(a, b) ((a).state == (b).state)
static inline int bit( int i, int k ) { return (i & (1 << (k - 1))) != 0; }

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static TYPE depth CALIGN, mask CALIGN;
static Tuple * Q CALIGN;								// volatile fields
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

WHOLESIZE QMAX( TYPE id, TYPE k ) {
	TYPE low = ((id >> (k - 1)) ^ 1) << (k - 1);
	TYPE high = MIN( (low | mask >> (depth - (k - 1))), N - 1 );
	WO( Fence(); );
	Tuple opp;
	for ( typeof(high) i = low; i <= high; i += 1 ) {
		opp.atom = Q[i].vatom;
		if ( L(opp) >= k ) return opp.atom;
	} // for
	return (Tuple){ { 0, 0 } }.atom;
} // QMAX

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	Tuple opp;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			for ( unsigned int k = 1; k <= (HALFSIZE)depth; k += 1 ) { // entry protocol, round
				opp.atom = QMAX( id, k );
				Fence();								// force store before more loads
				Q[id].vatom = L(opp) == k ? (Tuple){ { k, (HALFSIZE)(bit(id,k) ^ R(opp)) } }.atom : (Tuple){ {k, 1} }.atom;
				Fence();								// force store before more loads
				opp.atom = QMAX( id, k );
				Fence();								// force store before more loads
				Q[id].vatom = L(opp) == k ? (Tuple){ { k, (HALFSIZE)(bit(id,k) ^ R(opp)) } }.atom : Q[id].vatom;
				#if 0
			  wait:
				opp.atom = QMAX( id, k );
				Fence();								// force store before more loads
				if ( (L(opp) == k && (bit(id,k) ^ EQ(opp, Q[id]))) || L(opp) > k ) { Pause(); goto wait; }
				#else
				// modify to remove fence from loop
				Fence();								// force store before more loads
				while ( (L(opp) == k && (bit(id,k) ^ EQ(opp, Q[id]))) || L(opp) > k ) {
					Pause();
					opp.atom = QMAX( id, k );
				} // while
				#endif // 0
			} // for
			WO( Fence(); );

			randomThreadChecksum += CS( id );

			WO( Fence(); );
			Q[id].vatom = (Tuple){ { 0, 0 } }.atom;		// exit protocol

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		Fai( sumOfThreadChecksums, randomThreadChecksum );

		#ifdef FAST
		id = oid;
		#endif // FAST
		entries[r][id] = entry;
		Fai( Arrived, 1 );
		while ( stop != 0 ) Pause();
		Fai( Arrived, -1 );
	} // for
	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	depth = Clog2( N );									// maximal depth of binary tree
	int width = 1 << depth;								// maximal width of binary tree
	mask = width - 1;									// 1 bits for masking
	Q = Allocator( sizeof(typeof(Q[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {			// initialize shared data
		Q[i].atom = (Tuple){ { 0, 0 } }.atom;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)Q );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=PetersonT Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
