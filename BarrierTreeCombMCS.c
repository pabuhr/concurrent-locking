// John M. Mellor-Crummey and Michael L. Scott, Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors,
// ACM Transactions on Computer Systems, 9(1), February 1991, Fig. 9, p. 36

typedef	struct CALIGN Barrier {
	TYPE k, count;
	VTYPE locksense;
	struct Barrier * parent;
} Barrier_node;

typedef	struct {
	Barrier_node * bn;
} barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL TYPE sense = true;
#define BARRIER_CALL block( &b, p, &sense );

static inline void block_aux( Barrier_node * bn, TYPE * sense ) {
	if ( Fai( &bn->count, 1 ) == bn->k - 1 ) {
		if ( bn->parent ) {
			block_aux( bn->parent, sense );				// recursion
		} // if
		bn->count = 0;
		bn->locksense = ! bn->locksense;
	} // if
	await( bn->locksense == *sense );
} // block_aux

static inline void block( barrier * b, TYPE p, TYPE * sense ) {
	block_aux( &b->bn[p], sense );
	*sense = ! *sense;
} // barrier

//#define TESTING
#include "BarrierWorker.c"

const TYPE fanin = 2;

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b.bn = Allocator( sizeof(typeof(b.bn[0])) * N );
	b.bn[0] = (Barrier_node){ 1, 0, false, NULL };		// special case for root (no parent)
	for ( typeof(N) i = 1; i < N; i += 1 ) {
		// add child based on fanin structure (num children per node) of the tree
		b.bn[i] = (Barrier_node){ 1, 0, false, &b.bn[(i - 1) / fanin ] };
		// update fanin counter for parent (important for trees with a number of nodes that is not a power of fanin)
		b.bn[(i - 1) / fanin ].k += 1;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( b.bn );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierTreeCombMCS Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
