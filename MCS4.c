// John M. Mellor-Crummey and Michael L. Scott, Algorithm for Scalable Synchronization on Shared-Memory Multiprocessors,
// ACM Transactions on Computer Systems, 9(1), 1991, Fig. 7, p. 32

#include <stdbool.h>
#include <stdatomic.h>

typedef struct mcs_node {
	struct mcs_node * volatile next;
	VTYPE spin;
} MCS_node CALIGN;

typedef MCS_node * MCS_lock;

#define load( var, order ) atomic_load_explicit( var, order )
#define store( var, val, order ) atomic_store_explicit( var, val, order )
#define exchange( var, val, order ) atomic_exchange_explicit( var, val, order )
#define fetch_add( var, val ) atomic_fetch_add_explicit( var, val, memory_order_seq_cst )

inline void mcs_lock( MCS_lock * lock, MCS_node * node ) {
	store( &node->next, NULL, memory_order_relaxed );
#ifndef MCS_OPT1										// default option
	store( &node->spin, true, memory_order_relaxed );	// mark as waiting
#endif // MCS_OPT1
	MCS_node * prev = exchange( lock, node, memory_order_seq_cst );
  if ( SLOWPATH( prev == NULL ) ) return;				// no one on list ?
#ifdef MCS_OPT1
	atomic_store_explicit( &node->spin, true, memory_order_relaxed ); // mark as waiting
#endif // MCS_OPT1
	store( &prev->next, node, memory_order_release);	// add to list of waiting threads
	while ( load( &node->spin, memory_order_acquire ) ) Pause(); // busy wait on my spin variable
} // mcs_lock

inline void mcs_unlock( MCS_lock * lock, MCS_node * node ) {
#ifdef MCS_OPT2											// original, default option
	if ( FASTPATH( load( &node->next, memory_order_relaxed ) == NULL ) ) { // no one waiting ?
		MCS_node * old_tail = exchange( lock, NULL, memory_order_seq_cst );
  if ( SLOWPATH( old_tail == node ) ) return;
		MCS_node * usurper = exchange( lock, old_tail, memory_order_seq_cst );
		while ( load( &node->next, memory_order_relaxed ) == NULL) Pause(); // busy wait until my node is modified
		if ( usurper != NULL ) {
			store( &usurper->next, node->next, memory_order_release );
		} else {
			store( &node->next->spin, false, memory_order_release );
		} // if
	} else {
		store( &node->next->spin, false, memory_order_release );
	} // if
#else													// Scott book Figure 4.8
	MCS_node * succ = load( &node->next, memory_order_relaxed );
	if ( FASTPATH( succ == NULL ) ) {					// no one waiting ?
		MCS_node * old_tail = exchange( lock, NULL, memory_order_seq_cst );
  if ( SLOWPATH( old_tail == node ) ) return;
		MCS_node * usurper = exchange( lock, old_tail, memory_order_seq_cst );
		while ( (succ = load( &node->next, memory_order_relaxed ) ) == NULL) Pause(); // busy wait until my node is modified
		if ( usurper != NULL ) {
			store( &usurper->next, succ, memory_order_release );
		} else {
			store( &succ->spin, false, memory_order_release );
		} // if
	} else {
		store( &succ->spin, false, memory_order_release );
	} // if
#endif // MCS_OPT2
} // mcs_unlock

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static MCS_lock lock CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	MCS_node node;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			mcs_lock( &lock, &node );

			randomThreadChecksum += CS( id );

			mcs_unlock( &lock, &node );

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		fetch_add( &sumOfThreadChecksums, randomThreadChecksum );

		#ifdef FAST
		id = oid;
		#endif // FAST
		entries[r][id] = entry;
		fetch_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		fetch_add( &Arrived, -1 );
	} // for

	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	lock = NULL;
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=MCS4 Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
