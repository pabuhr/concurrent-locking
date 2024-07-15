// John M. Mellor-Crummey and Michael L. Scott, Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors,
// ACM Transactions on Computer Systems, 9(1), February 1991, Fig. 5, p. 30

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

#include "MCS.h"

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
	lock = NULL;
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=MCS Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
