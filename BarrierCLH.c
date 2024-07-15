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
	Node * head;
    #ifndef ATOMIC
	Node * volatile
    #else
    Node * _Atomic
    #endif // ! ATOMIC
		swap;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline void block( Barrier * b ) {
	Node myNode = { .next = NULL, .cnt = 0 };
	Node * prev = Fas( b->swap, &myNode );

	if ( ! prev ) {										// special case for 1st node
		b->head = &myNode;
		myNode.cnt = 1;
	} else {
		prev->next = &myNode;
	} // if

	await( myNode.cnt != 0 );							// wait for acknowledgement from predecessor

	if ( myNode.cnt == N ) {							// quorum reached => trigger barrier
		// CALL ACTION CALLBACK BEFORE TRIGGERING BARRIER
		Node * curr = b->head;
		for ( TYPE i = 0; i < N - 1; i += 1 ) {
			prev = curr;
			curr = curr->next;
			prev->cnt = N;
		} // for

		// If swap is still me, then myNode.next may not be set. Attempt to reset swap to null and cut the chain.
		if ( b->swap == &myNode ) {
			if ( Cas( b->swap, &myNode.next, NULL ) ) return;
		} // if

		// Otherwise cooperate and update head to start next epoch.
		await( myNode.next );
		b->head = myNode.next;
		myNode.next->cnt = 1;
		Fence();
		return;
	} // if

	await( myNode.next );								// quorum not reached
	myNode.next->cnt = myNode.cnt + 1;					// acknowledge successor
	await( myNode.cnt == N );							// either quorum not reached or signal pending
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	b = (Barrier){ .head = NULL, .swap = NULL };
	worker_ctor();
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierCLH Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
