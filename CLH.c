// Peter Magnusson, Anders Landin, Erik Hagersten, Queue Locks on Cache Coherent Multiprocessors, Proceedings of 8th
// International Parallel Processing Symposium (IPPS), 1994, Figure 2 (LH lock).

#include <stdbool.h>

typedef VTYPE qnode CALIGN;
typedef qnode * qnode_ptr;
typedef qnode_ptr CLH_lock;

#define THREADLOCAL /* fastest version */

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static CLH_lock lock CALIGN;
#ifdef THREADLOCAL
static __thread qnode_ptr node CALIGN;					// alternative location
#define NPARM( addr )
#define NARG( addr )
#define STAR
#else
#define NPARM( star ) , qnode_ptr star node
#define NARG( addr ) , addr node
#define STAR *
#endif // THREADLOCAL
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static inline void clh_lock( CLH_lock * lock, qnode_ptr * prev  NPARM() ) {
	*node = false;
	*prev = Fas( lock, node );
	await( **prev );
	WO( Fence(); );
} // clh_lock

static inline void clh_unlock( qnode_ptr prev  NPARM(*) ) {
	WO( Fence(); );
	* STAR node = true;
	STAR node = prev;
} // clh_unlock

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	#ifndef THREADLOCAL
	qnode_ptr node;
	#endif // ! THREADLOCAL

	node = Allocator( sizeof( qnode ) );				// cache alignment
	qnode_ptr prev;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			clh_lock( &lock, &prev  NARG() );

			randomThreadChecksum += CS( id );

			clh_unlock( prev  NARG(&) );

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

	#ifndef THREADLOCAL
	free( (TYPE *)node );									// remove volatile
	#endif // ! THREADLOCAL

	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	lock = Allocator( sizeof( qnode ) );				// cache alignment
	*lock = true;
} // ctor

void __attribute__((noinline)) dtor() {
	free( (TYPE *)lock );								// remove volatile
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=CLH Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
