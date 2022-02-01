// Michael L. Scott Shared-Memory Synchronization book, Figure 4.12, Chapter 4, page 14

#include <stdbool.h>

typedef struct qnode_t {
	#ifndef ATOMIC
	struct qnode_t * volatile
	#else
	_Atomic(struct qnode_t *)
	#endif // ! ATOMIC
		prev;

	#ifndef ATOMIC
	volatile bool
	#else
	_Atomic(bool)
	#endif // ! ATOMIC
		succ_must_wait;
} qnode CALIGN;

typedef qnode * qnode_ptr;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static qnode_ptr tail CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define await( E ) while ( ! (E) ) Pause()

static inline void clh_lock( qnode_ptr p ) {
	p->succ_must_wait = true;
	qnode_ptr pred = p->prev = __atomic_exchange_n( &tail, p, __ATOMIC_SEQ_CST );
	await( ! pred->succ_must_wait );
	WO( Fence(); );
} // clh_lock

static inline void clh_unlock( qnode_ptr * pp ) {		// "**" output parameter
	WO( Fence(); );
	qnode_ptr pred = (*pp)->prev;
	(*pp)->succ_must_wait = false;
	*pp = pred;
} // clh_unlock

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	qnode_ptr node_ptr = Allocator( sizeof( qnode ) );	// cache alignment

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		*node_ptr = (qnode){ NULL, false };

		for ( entry = 0; stop == 0; entry += 1 ) {
			clh_lock( node_ptr );

			randomThreadChecksum += CriticalSection( id );

			clh_unlock( &node_ptr );

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

	free( node_ptr );

	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	tail = Allocator( sizeof( qnode ) );				// cache alignment
	*tail = (qnode){ NULL, false };
} // ctor

void __attribute__((noinline)) dtor() {
	free( (TYPE *)tail );								// remove volatile
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=CLH_Scott_F12 Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
