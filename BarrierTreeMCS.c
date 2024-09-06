// John M. Mellor-Crummey and Michael L. Scott, Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors,
// ACM Transactions on Computer Systems, 9(1), February 1991, Fig. 12, p. 42

enum { FANIN = 4 };

typedef union {
	VHALFSIZE all;
	VBYTESIZE flags[FANIN];
} Fanin;

typedef struct CALIGN {
	VBYTESIZE * parent;
	VBYTESIZE * children[2];
	Fanin has_child;
	Fanin child_not_ready;
	VBYTESIZE parent_sense;
	BYTESIZE dummy;
} Tree_node;

typedef struct {
	Tree_node * tree;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL TYPE sense = true
#define BARRIER_CALL block( &b, p, &sense );

static inline void block( Barrier * b, TYPE p, TYPE * sense ) {
	Tree_node * tree_node = &b->tree[p];				// optimizations (compiler probably does it)
	TYPE lsense = *sense;

	await( ! tree_node->child_not_ready.all );			// await for all fan-in to be false
	tree_node->child_not_ready.all = tree_node->has_child.all;
	*tree_node->parent = false;
	Fence();
	// Only compiler fencing needed (volatile). Hardware fencing would only be necessary if the processor is able to
	// hoist the entire await loop, which is unlikely. That is, any number of loads of parent_sense can occur in the
	// loop before the store to parent occurs and some other thread breaks the loop.
	// Fence(); // add if needed

	// if not root, wait until my parent signals wakeup
	if ( p != 0 ) {
		await( lsense == tree_node->parent_sense );
		// CALL ACTION CALLBACK BEFORE TRIGGERING BARRIER
	} // if

	// signal children in wakeup tree
	*tree_node->children[0] = lsense;
	*tree_node->children[1] = lsense;
	*sense = ! lsense;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	b.tree = Allocator( sizeof(Tree_node) * N );
	for ( TYPE i = 0; i < N; i++ ) {
		b.tree[i].parent_sense = false;
		for ( int j = 0; j < FANIN; j++ ) {
			// https://www.cs.rochester.edu/research/synchronization/pseudocode/ss.html
			// off-by-one error fixed from the pseudocode in paper
			b.tree[i].has_child.flags[j] = FANIN * i + j + 1 < N;
			b.tree[i].child_not_ready.flags[j] = b.tree[i].has_child.flags[j];
		} // for
	
		if ( i == 0 ) b.tree[i].parent = (VBYTESIZE *)(&b.tree[i].dummy);
		else b.tree[i].parent = &b.tree[(i - 1) / FANIN].child_not_ready.flags[(i - 1) % FANIN];

		if ( 2 * i + 2 >= N ) b.tree[i].children[1] = (VBYTESIZE *)(&b.tree[i].dummy);
		else b.tree[i].children[1] = &b.tree[2 * i + 2].parent_sense;

		if ( 2 * i + 1 >= N ) b.tree[i].children[0] = (VBYTESIZE *)(&b.tree[i].dummy);
		else b.tree[i].children[0] = &b.tree[2 * i + 1].parent_sense;
	} // for
	worker_ctor();
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
	free( b.tree );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierTreeMCS Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
