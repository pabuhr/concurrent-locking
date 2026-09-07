#include "BarrierCallback.h"
#include "BarrierQuorum.h"

typedef struct CALIGN {
	TYPE CALIGN group;
	TYPE CALIGN threads;
	VTYPE CALIGN low;
	TYPE CALIGN high;
	QUORUM_DECL;
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b )

typedef enum { Leader, Follower, QuorumFail } BarrierRet;

static inline BarrierRet block( Barrier * b ) {
	QUORUM_ENTER( b );									// Fence
	CBSTART();											// must be first
	TYPE counter = Fai( b->high, 1 );					// take ticket => establish order

	if ( LIKELY( counter % b->group != b->group - 1 ) ) { // not leader ?
		await( counter < b->low );						// wait for quorum, potential overflow problem
		QUORUM_CHECK( b );								// check for quorum failure and return QuorumFail
		return Follower;
	} // if
	await( counter == b->low + b->group - 1 );			// wait for quorum
	CBEND();											// must appear in safe location
	b->low = counter + 1;
	QUORUM_EXIT( b );									// Fence
	return Leader;
} // block

static inline void done( Barrier * b ) {
	QUORUM_DONE( b );
} // done

#include "BarrierWorkerG.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	// TYPE G = N == 1 ? 1 : N / 2;
	// fprintf( stderr, " G %d ", 4 );
	enum { G = 4 };
	fprintf( stderr, " G %d ", G );
	b = (Barrier){ .group = G, .threads = N, .low = 0, .high = 0 CBINIT() };
	QUORUM_CTOR( &b );
} // ctor

void __attribute__((noinline)) dtor() {
	QUORUM_DTOR( &b );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierG Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
