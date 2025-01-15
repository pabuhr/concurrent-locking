// Wim H. Hesselink and Peter A. Buhr, MCSH, a Lock with the Standard Interface, ACM Transactions on Parallel Computing,
// 10(2), June 2023, pp 1-23

// There are two versions of the algorithm: the original in the TOPC's paper and with modifications by Ting-Ching Li,
// National Chengchi University, Taipei, Taiwan.

#include <stdbool.h>

typedef struct mcsh_node {
	#ifndef ATOMIC
	struct mcsh_node * volatile
	#else
	_Atomic(struct mcsh_node *)
	#endif // ! ATOMIC
		next;
	VTYPE locked;
} MCSH_node CALIGN;

#ifndef ATOMIC
typedef MCSH_node *
#else
typedef _Atomic(MCSH_node *)
#endif // ! ATOMIC
	MCSH_node_ptr;

typedef struct {
	MCSH_node_ptr tail;
	MCSH_node_ptr msg;
	VTYPE flag;
} NMCS_lock CALIGN;

static inline void mcs_lock( NMCS_lock * lock ) {
	MCSH_node mm = { .next = NULL, .locked = true };
	MCSH_node_ptr prev = Fas( lock->tail, &mm );

	if ( FASTPATH( prev == NULL ) ) {
		await( lock->flag );
		lock->flag = false;								// Li modification
	} else {
		prev->next = &mm;
		// Possible correctness issue: the MCSH_node is allocated on the stack so escape analysis thinks the
		// initialization cannot leak out before the fence, therefore the compiler might move all the initialization
		// behind the fence (thereby making the fence ineffective). Jonas Oberhauser
		await( ! mm.locked );
	} // if

	// WO( Fence(); );									// original paper
	// lock->flag = false;
	WO( Fence(); );
	MCSH_node_ptr succ = mm.next;
	if ( FASTPATH( succ == NULL ) ) {
		if ( FASTPATH( ! Cas( lock->tail, &mm, NULL ) ) ) {
			await( (succ = mm.next) != NULL );
		} // if
	} // if
	WO( Fence(); );
	lock->msg = succ;
} // mcs_lock

// The first two lines of release must not be swapped (see handware Fence), because, if flag holds before msg is read, a
// new thread may be able to modify msg.

static inline void mcs_unlock( NMCS_lock * lock ) {
	MCSH_node_ptr succ = lock->msg;
	WO( Fence(); );
	// lock->flag = true;								// original paper
	if ( FASTPATH( succ != NULL ) ) {
		succ->locked = false;
	} else {											// Li modification
		lock->flag = true;
	} // if
} // mcs_unlock

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static NMCS_lock lock CALIGN;
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

			mcs_lock( &lock );

			randomThreadChecksum += CS( id );

			mcs_unlock( &lock );

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
	lock.tail = NULL;
	lock.flag = true;
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=MCSH Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
