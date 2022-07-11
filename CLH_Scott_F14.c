// Michael L. Scott Shared-Memory Synchronization book, Figure 4.14, Chapter 4, page 16

#include <stdbool.h>

typedef struct CALIGN {
	VTYPE qnode;
} qnode;

typedef struct CALIGN {
	qnode *	qnode_ptr;
} qnode_ptr;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static qnode * initial_thread_qnodes CALIGN;
static qnode_ptr * thread_qnode_ptrs CALIGN;
static qnode dummy CALIGN = { false };
static qnode_ptr tail CALIGN = { &dummy };
static qnode_ptr head CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static inline void clh_lock( TYPE id ) {
	qnode_ptr p = thread_qnode_ptrs[id];
	(*p.qnode_ptr).qnode = true;
	qnode_ptr pred;
	pred.qnode_ptr = Fas( &(tail.qnode_ptr), p.qnode_ptr );
	thread_qnode_ptrs[id] = pred;
	await( ! (*pred.qnode_ptr).qnode );
	head = p;
	WO( Fence(); );
} // clh_lock

static inline void clh_unlock() {
	WO( Fence(); );
	(*head.qnode_ptr).qnode = false;
} // clh_unlock

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			clh_lock( id );

			randomThreadChecksum += CS( id );

			clh_unlock();

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
	initial_thread_qnodes = Allocator( sizeof( qnode ) * N ); // cache alignment
	thread_qnode_ptrs = Allocator( sizeof( qnode_ptr ) * N ); // cache alignment
	for ( typeof(N) i = 0; i < N; i += 1 ) {			// initialize shared data
		thread_qnode_ptrs[i].qnode_ptr = &initial_thread_qnodes[i];
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)thread_qnode_ptrs );
	free( (void *)initial_thread_qnodes );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=CLH_Scott_F14 Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
