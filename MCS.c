// John M. Mellor-Crummey and Michael L. Scott, Algorithm for Scalable Synchronization on Shared-Memory Multiprocessors,
// ACM Transactions on Computer Systems, 9(1), 1991, Fig. 5, p. 30

// Architectural fences are unnecssary on TSO for MCS and CLH. An atomic instruction is a hardware fence, hence the only
// reordering is store-load: stores in program order followed by loads being inverted in memory order by the CPU because
// of a store buffer.  (Peter Sewell et al showed all TSO reorderings, even those not arising from the store buffer, are
// attributable to a simple store buffer model, which is a helpful simplification).  In the lock() path, the store to
// clear the flag in the node happens before the atomic preventing movement and that store cannot be reorder in any
// meaningful way.
//
// The other interesting store releases the lock.  Loads after the CS in program order can by lifted by the CPU up above
// the release store into the CS, but these are benign, as it is always safe to move accesses that are outside and after
// the CS "up" into the CS.  Critically, loads and stores in the CS body itself cannot be reordered past the store that
// releases the lock.  Hence, accesses can only "leak" one direction: from outside the CS into the CS, but not the other
// direction. That covers acquire-release memory ordering.
//
// The other concern is compiler-based reordering, but the store to the release the lock should be to a volatile/atomic,
// which protects against compile-time movement.

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

inline void mcs_lock( MCS_lock * lock, MCS_node * node ) {
	WO( Fence(); ); // 1
	node->next = NULL;
#ifndef MCS_OPT1										// default option
	node->spin = true;									// alternative position and remove fence below
	WO( Fence(); ); // 2
#endif // MCS_OPT1
	MCS_node * prev = __atomic_exchange_n( lock, node, __ATOMIC_SEQ_CST );
  if ( SLOWPATH( prev == NULL ) ) return;				// no one on list ?
#ifdef MCS_OPT1
	node->spin = true;									// mark as waiting
	WO( Fence(); ); // 3
#endif // MCS_OPT1
	prev->next = node;									// add to list of waiting threads
	WO( Fence(); ); // 4
	while ( node->spin == true ) Pause();				// busy wait on my spin variable
	WO( Fence(); ); // 5
} // mcs_lock

inline void mcs_unlock( MCS_lock * lock, MCS_node * node ) {
	WO( Fence(); ); // 6
#ifndef MCS_OPT2										// original, default option
	if ( FASTPATH( node->next == NULL ) ) {				// no one waiting ?
		MCS_node * temp = node;							// copy because exchange overwrites expected
  if ( __atomic_compare_exchange_n( lock, &temp, NULL, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST ) ) return; // Fence
		while ( node->next == NULL ) Pause();			// busy wait until my node is modified
	} // if
	WO( Fence(); ); // 7
	node->next->spin = false;							// stop their busy wait
#else													// Dice option
	MCS_node * succ = node->next;
	if ( FASTPATH( succ == NULL ) ) {					// no one waiting ?
		// node is potentially at the tail of the MCS chain 
		MCS_node * temp = node;							// copy because exchange overwrites expected
  if ( __atomic_compare_exchange_n( lock, &temp, NULL, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST ) ) return; // Fence
		// Either a new thread arrived in the LD-CAS window above or fetched a false NULL
		// as the successor has yet to store into node->next.
		while ( (succ = node->next) == NULL ) Pause();	// busy wait until my node is modified
	} // if
	WO( Fence(); ); // 8
	succ->spin = false;
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

	MCS_node node;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

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
