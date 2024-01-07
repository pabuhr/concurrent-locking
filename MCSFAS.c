// John M. Mellor-Crummey and Michael L. Scott, Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors,
// ACM Transactions on Computer Systems, 9(1), February 1991, Fig. 7, p. 32, not FIFO but runs like FIFO.

#include <stdbool.h>

typedef struct mcs_node {
	#ifndef ATOMIC
	struct mcs_node * volatile next;
	#else
	_Atomic(struct mcs_node *) next;
	#endif // ! ATOMIC
	VTYPE spin;
} MCS_node CALIGN;

#ifndef ATOMIC
typedef MCS_node * MCS_lock;
#else
typedef _Atomic(MCS_node *) MCS_lock;
#endif // ! ATOMIC

static inline void mcs_lock( MCS_lock * lock, MCS_node * node ) {
	node->next = NULL;

#ifndef MCS_OPT1										// default option
	node->spin = true;									// alternative position and remove fence below
	WO( Fence(); ); // 1
#endif // MCS_OPT1

	MCS_node * prev = Fas( lock, node );

  if ( SLOWPATH( prev == NULL ) ) return;				// no one on list ?

#ifdef MCS_OPT1
	node->spin = true;									// mark as waiting
	WO( Fence(); ); // 2
#endif // MCS_OPT1

	prev->next = node;									// add to list of waiting threads
	WO( Fence(); ); // 3

	#ifndef MPAUSE
	while ( node->spin == true ) Pause();				// busy wait on my spin variable
	#else
	MPause( node->spin, == true );						// busy wait on my spin variable
	#endif // MPAUSE
	WO( Fence(); ); // 4
} // mcs_lock

static inline void mcs_unlock( MCS_lock * lock, MCS_node * node ) {
	WO( Fence(); ); // 5
#ifdef MCS_OPT2											// original, default option
	if ( FASTPATH( node->next == NULL ) ) {				// no one waiting ?
		MCS_node * old_tail = Fas( lock, NULL );

  if ( SLOWPATH( old_tail == node ) ) return;

  		MCS_node * usurper = Fas( lock, old_tail );
		#ifndef MPAUSE
		while ( node->next == NULL ) Pause();			// busy wait until my node is modified
		#else
		MPause( node->next, == NULL );					// busy wait until my node is modified
		#endif // MPAUSE
		WO( Fence(); ); // 6

		if ( usurper != NULL ) {
			usurper->next = node->next;
		} else {
			node->next->spin = false;
		} // if
	} else {
		WO( Fence(); ); // 7
		node->next->spin = false;
	} // if
#else													// Scott book Figure 4.8
	MCS_node * succ = node->next;
	if ( FASTPATH( succ == NULL ) ) {					// no one waiting ?
		MCS_node * old_tail = Fas( lock, NULL );

  if ( SLOWPATH( old_tail == node ) ) return;

		MCS_node * usurper = Fas( lock, old_tail );
		#ifndef MPAUSE
		while ( (succ = node->next) == NULL ) Pause();	// busy wait until my node is modified
		#else
		MPauseS( succ =, node->next, == NULL );			// busy wait until my node is modified
		#endif // MPAUSE
		WO( Fence(); ); // 6

		if ( usurper != NULL ) {
			usurper->next = succ;
		} else {
			succ->spin = false;
		} // if
	} else {
		WO( Fence(); ); // 7
		succ->spin = false;
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
	lock = NULL;
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=MCSFAS Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
