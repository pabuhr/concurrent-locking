// Michael L. Scott Shared-Memory Synchronization book, Figure 4.10, Chapter 4, page 12

#include <stdbool.h>

typedef struct mcs_node {
	#ifndef ATOMIC
	struct mcs_node * volatile tail, * volatile next;
	#else
	_Atomic(struct mcs_node *) tail, next;				// distributes atomic
	#endif // ! ATOMIC
} MCS_node CALIGN;

#define waiting ((MCS_node *)1)

static inline void mcs_lock( MCS_node * lock ) {
	for ( ;; ) {
		MCS_node * prev = lock->tail;
		if ( prev == NULL ) {
			if ( Cas( (lock->tail), NULL, lock ) ) break; // Fence
		} else {
			MCS_node n = { waiting, NULL };
			if ( Cas( (lock->tail), prev, &n ) ) {		// Fence
				prev->next = &n;
				await( n.tail != waiting );
				MCS_node * succ = n.next;
				if ( succ == NULL ) {
					lock->next = NULL;
					if ( ! Cas( (lock->tail), &n, lock ) ) { // Fence
						await( (succ = n.next) != NULL );
						WO( Fence(); );
						lock->next = succ;
					} // if
					break;
				} else {
					lock->next = succ;
					break;
				} // if
			} // if
		} // if
	} // for
	WO( Fence(); );
} // mcs_lock

static inline void mcs_unlock( MCS_node * lock ) {
	WO( Fence(); );
	MCS_node * succ = lock->next;
	if ( succ == NULL ) {
		if ( Cas( (lock->tail), lock, NULL ) ) return; // Fence
		await( (succ = lock->next) != NULL );
		WO( Fence(); );
	} // if
	succ->tail = NULL;
} // mcs_unlock

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static MCS_node Lock CALIGN = { NULL, NULL };
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

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

			mcs_lock( &Lock );

			randomThreadChecksum += CS( id );

			mcs_unlock( &Lock );

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
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=MCSK42 Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
