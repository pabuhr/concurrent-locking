// Travis S. Craig, Building FIFO and priority-queueing spin locks from atomic swap. Technical Report TR 93-02-02,
// University of Washington, Department of Computer Science and Engineering, February 1993, Appendix A.
// 
// Peter Magnusson, Anders Landin, Erik Hagersten, Queue Locks on Cache Coherent Multiprocessors, Proceedings of 8th
// International Parallel Processing Symposium (IPPS), 1994, Figure 2 (LH lock).

#include <stdbool.h>

typedef VTYPE qnode CALIGN;

#ifndef ATOMIC
typedef qnode *
#else
typedef _Atomic(qnode *)
#endif // ! ATOMIC
	qnode_ptr;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static qnode_ptr tail CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	qnode_ptr n = Allocator( sizeof( qnode ) );			// cache alignment

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			*n = false;
			qnode_ptr prev = Faa( &tail, n );
			await( *prev );
			WO( Fence(); );

			randomThreadChecksum += CriticalSection( id );

			WO( Fence(); );
			*n = true;
			n = prev;

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

	free( (TYPE *)n );									// remove volatile

	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	tail = Allocator( sizeof( qnode ) );				// cache alignment
	*tail = true;
} // ctor

void __attribute__((noinline)) dtor() {
	free( (TYPE *)tail );								// remove volatile
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=CLH Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
