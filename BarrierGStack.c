#include "BarrierCallback.h"
#include "BarrierQuorum.h"

typedef struct CALIGN {
	VTYPE gate;
    VSTYPE count;										// position in stack
} Pair; 

typedef struct CALIGN {
	STYPE CALIGN group;
	TYPE CALIGN threads;
	Pair * head CALIGN;
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
	QUORUM_ENTER( b );
	CBSTART();											// must be first

	STYPE prvcnt;
	Pair ee = { .gate = false, .count = -1 };
	Pair * prev = Fas( b->head, &ee );

	if ( UNLIKELY( prev == NULL ) ) {
		if ( b->group == 1 ) { b->head = NULL; return Leader; }
		ee.count = 1;
		Fence();
		await( ee.gate );
		QUORUM_CHECK( b );
		return Follower;
	} // if

	do { prvcnt = prev->count; } while ( prvcnt < 0 );
	ee.count = prvcnt < (STYPE)b->group - 1 ? 1 + prvcnt : 0;

	if ( prvcnt == 0 ) {
		prev->gate = true ;
		Fence();
		await( ee.gate );
		QUORUM_CHECK( b );
		return Follower;
	} // if

	if ( LIKELY( prvcnt < (STYPE)b->group - 1 ) ) {
		await( ee.gate );
		prev->gate = true;
		Fence();
		QUORUM_CHECK( b );
		return Follower;
	} // if

	CBEND();											// must appear in safe location
	if ( Cas( b->head, &ee, NULL ) ) {
		prev->gate = true;
		Fence();
		QUORUM_EXIT( b );
		return Leader;
	} // if

	await( ee.gate );
	prev->gate = true;
	Fence();
	QUORUM_EXIT( b );
	return Leader;
} // block

static inline void done( Barrier * b, TYPE p ) {
	QUORUM_DONE( b, p );
} // done

#include "BarrierWorker3.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	enum { G = 4 };
	fprintf( stderr, " G %d ", G );
	b = (Barrier){ .group = G, .threads = N, .head = NULL CBINIT() };
	QUORUM_CTOR( b );
} // ctor

void __attribute__((noinline)) dtor() {
	QUORUM_DTOR( b );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierGStack Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
