// John M. Mellor-Crummey and Michael L. Scott, Algorithm for Scalable Synchronization on Shared-Memory Multiprocessors,
// ACM Transactions on Computer Systems, 9(1), 1991, Fig. 6, p. 30

#include <stdbool.h>

typedef struct mcs_node {								// cache align node at declaration
	#ifndef ATOMIC
	struct mcs_node * volatile next;
	#else
	_Atomic(struct mcs_node *) next;
	#endif // ! ATOMIC
	VTYPE spin;
} MCS_node;
#ifndef ATOMIC
typedef struct mcs_node * MCS_lock;
#else
typedef _Atomic(MCS_node *) MCS_lock;
#endif // ! ATOMIC

void mcs_lock( MCS_lock * lock, MCS_node * node ) {
	MCS_node * pred;
	node->next = NULL;
	// node->spin = 1;									// alternative position and remove fence below
	WO( Fence(); );
	pred = __atomic_exchange_n( lock, node, __ATOMIC_ACQUIRE );

	if ( FASTPATH( pred != NULL ) ) {					// someone on list ?
		node->spin = 1;									// mark as waiting
		WO( Fence(); );
		pred->next = node;								// add to list of waiting threads
		while ( node->spin == 1 ) Pause();				// busy wait on my spin variable
		WO( Fence(); );
	} // if
} // mcs_lock

void mcs_unlock( MCS_lock * lock, MCS_node * node ) {
#if 0  // original
	if ( node->next == NULL ) {							// no one waiting ?
		MCS_node * temp = node;							// copy because exchange overwrites expected
  if ( __atomic_compare_exchange_n( lock, &temp, NULL, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST ) ) return; // Fence
		while ( node->next == NULL ) Pause();			// busy wait until my node is modified
	} // if
	WO( Fence(); );
	node->next->spin = 0;								// stop their busy wait
#else  // Dice
	MCS_node * succ = node->next;
	if ( FASTPATH( succ == NULL ) ) {					// no one waiting ?
		// node is potentially at the tail of the MCS chain 
		MCS_node * temp = node;							// copy because exchange overwrites expected
  if ( __atomic_compare_exchange_n( lock, &temp, NULL, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST ) ) return; // Fence
		// Either a new thread arrived in the LD-CAS window above or fetched a false NULL
		// as the successor has yet to store into node->next.
		while ( (succ = node->next) == NULL ) Pause();	// busy wait until my node is modified
	} // if
	WO( Fence(); );
	succ->spin = 0;
#endif // 0
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

	MCS_node node CALIGN;								// sufficient to cache align node

	for ( int r = 0; r < RUNS; r += 1 ) {
		uint32_t randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			mcs_lock( &lock, &node );

			randomThreadChecksum += CriticalSection( id );

			mcs_unlock( &lock, &node );

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
	lock = NULL;
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=MCS Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
