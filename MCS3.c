// John M. Mellor-Crummey and Michael L. Scott, Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors,
// ACM Transactions on Computer Systems, 9(1), February 1991, Fig. 5, p. 30

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
#define CAS( var, comp, val, order1, order2 ) atomic_compare_exchange_strong_explicit( var, comp, val, order1, order2 )
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

	#ifndef MPAUSE
	while ( load( &node->spin, memory_order_acquire ) == true ) Pause(); // busy wait on my spin variable
	#else
	MPause( node->spin, == true );						// busy wait on my spin variable
	#endif // MPAUSE
} // mcs_lock

inline void mcs_unlock( MCS_lock * lock, MCS_node * node ) {
#ifdef MCS_OPT2											// original, default option
	if ( FASTPATH( load( &node->next, memory_order_relaxed ) == NULL ) ) { // no one waiting ?
		MCS_node * temp = node;							// copy because exchange overwrites expected

  if ( SLOWPATH( CAS( lock, &temp, NULL, memory_order_release, memory_order_relaxed ) ) ) return;

		#ifndef MPAUSE
		while ( load( &node->next, memory_order_relaxed ) == NULL ) Pause(); // busy wait until my node is modified
		#else
		MPause( node->next, == NULL );					// busy wait until my node is modified
		#endif // MPAUSE
	} // if

	// MCS_node * next = atomic_load_explicit( &node->next, memory_order_acquire );
	store( &node->next->spin, false, memory_order_release );
#else													// Scott book Figure 4.8
	MCS_node * succ = load( &node->next, memory_order_relaxed );
	if ( FASTPATH( succ == NULL ) ) {					// no one waiting ?
		// node is potentially at the tail of the MCS chain 
		MCS_node * temp = node;							// copy because exchange overwrites expected
  if ( SLOWPATH( CAS( lock, &temp, NULL, memory_order_release, memory_order_relaxed ) ) ) return;

		#ifndef MPAUSE
		while ( (succ = load( &node->next, memory_order_relaxed ) ) == NULL) Pause(); // busy wait until my node is modified
		#else
		MPauseS( succ =, node->next, == NULL );			// busy wait until my node is modified
		#endif // MPAUSE
	} // if
	store( &succ->spin, false, memory_order_release );
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
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=MCS3 Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
