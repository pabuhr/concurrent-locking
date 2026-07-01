// John M. Mellor-Crummey and Michael L. Scott, Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors,
// ACM Transactions on Computer Systems, 9(1), February 1991, Fig. 9, p. 36

#include "BarrierCallback.h"

typedef struct CALIGN Barrier_node {
	TYPE k, count;
	VTYPE locksense;
	struct Barrier_node * parent;
} Barrier_node;

typedef	struct {
	CB( TYPE CALIGN group; )
	Barrier_node * bn;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL TYPE sense = true;
#define BARRIER_CALL block( &b, p, &sense );

static inline bool block_aux( CB( Barrier * b,) Barrier_node * bn, TYPE sense ) {
	bool ret;
	if ( UNLIKELY( Fai( bn->count, 1 ) == bn->k - 1 ) ) {
		if ( bn->parent ) {								// last one to reach this node
			ret = block_aux( CB( b,) bn->parent, sense ); // RECURSION
		} else {
			ret = true;
			CBEND();									// must appear in safe location
		} // if

		bn->count = 0;									// prepare for next barrier
		bn->locksense = ! bn->locksense;				// release waiting processors
	} else {
		ret = false;
	} // if
	await( bn->locksense == sense );
	return ret;
} // block_aux

static inline bool block( Barrier * b, TYPE p, TYPE * sense ) {
	CBSTART();											// must be first
	TYPE lsense = *sense;								// optimization (compiler probably does it)
	bool ret = block_aux( CB( b,) &b->bn[p], lsense );
	*sense = ! lsense;
	return ret;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	enum { FANIN = 2 };

	worker_ctor();
	b = (Barrier){ CB( .group = N, ) .bn = Allocator( sizeof(typeof(b.bn[0])) * N ) CBINIT() };
	b.bn[0] = (Barrier_node){ 1, 0, false, NULL };		// special case for root (no parent)
	for ( typeof(N) i = 1; i < N; i += 1 ) {
		// add child based on fanin structure (num children per node) of the tree
		b.bn[i] = (Barrier_node){ 1, 0, false, &b.bn[(i - 1) / FANIN ] };
		// update fanin counter for parent (important for trees with a number of nodes that is not a power of fanin)
		b.bn[(i - 1) / FANIN ].k += 1;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( b.bn );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierTreeCombMCS Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
