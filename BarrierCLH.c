// Colby wrote but discarded because Dave's is better.

#include "BarrierCallback.h"

typedef struct Node {
    #ifndef ATOMIC
    struct Node * volatile
    #else
    struct Node * _Atomic
    #endif // ! ATOMIC
        next;
	VTYPE cnt;
} Node;

typedef struct {
	TYPE CALIGN group;
	Node * head;
    #ifndef ATOMIC
	Node * volatile
    #else
    Node * _Atomic
    #endif // ! ATOMIC
		swap;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline bool block( Barrier * b ) {
	CBSTART();											// must be first
	Node myNode = { .next = NULL, .cnt = 0 };
	Node * prev = Fas( b->swap, &myNode );

	if ( ! prev ) {										// special case for 1st node
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wdangling-pointer"	// Mute g++
		b->head = &myNode;
		#pragma GCC diagnostic pop
		myNode.cnt = 1;
	} else {
		prev->next = &myNode;
	} // if

	await( myNode.cnt != 0 );							// wait for acknowledgement from predecessor

	if ( myNode.cnt == b->group ) {						// quorum reached => trigger barrier
		CBEND();										// must appear in safe location

		Node * curr = b->head;
		for ( TYPE i = 0; i < b->group - 1; i += 1 ) {
			prev = curr;
			curr = curr->next;
			prev->cnt = b->group;
		} // for

		// If swap is still me, then myNode.next may not be set. Attempt to reset swap to null and cut the chain.
		if ( b->swap == &myNode ) {
			if ( Cas( b->swap, &myNode.next, NULL ) ) return true;
		} // if

		// Otherwise cooperate and update head to start next epoch.
		await( myNode.next );
		b->head = myNode.next;
		myNode.next->cnt = 1;
		Fence();
		return true;
	} // if

	await( myNode.next );								// quorum not reached
	myNode.next->cnt = myNode.cnt + 1;					// acknowledge successor
	await( myNode.cnt == b->group );					// either quorum not reached or signal pending
	return false;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .head = NULL, .swap = NULL CB(, .callback = NULL ) };
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierCLH Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
