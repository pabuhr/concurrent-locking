// Debra Hensgen, Raphael Finkel, and Udi Manber, Two Algorithms for Barrier Synchronization, International Journal
// of Parallel Programming, Vol. 17, No. 1, 1988 pp. 11-13

typedef	struct {
	VTYPE ** Answers;									// flags waited on in each step of the tournament
	TYPE ** Opponent;									// table of tournament opponents
	VTYPE Announcement[2];								// global announcement flags to hold threads until after tournament
	VTYPE BarrierCount;									// which announcement is being used for current tournament
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b, p );

static inline void block( Barrier * b, TYPE p ) {
	TYPE power = 1, Clog2N = Clog2( N ), LocalBarrierCount;

	LocalBarrierCount = b->BarrierCount;
	for ( typeof(N) instance = 0; instance < Clog2N; instance += 1 ) {
	  if ( p % power != 0 ) {
			break;										// break out of loop; we are no longer active
		} // exit
		if ( p > b->Opponent[p][instance] ) {			// loser
			b->Answers[ b->Opponent[p][instance] ][instance] = true;
			Fence();
		} else if ( b->Opponent[p][instance] >= N ) {
			// win by default if no opponent
		} else {										// winner
			// The following two lines are incorrectly indexed in the paper as b->Answers[b->Opponent[p][instance]][instance].
			// Instead, they should be indexed as b->Answers[p][instance], since the loser sets the opponent flag (the
			// winner's flag). Hence, the winner must check their own flag, if it is set([p]), NOT the flag of the loser
			// (Opponent[p][instance]), since the loser's flag is never set.
			await( b->Answers[p][instance] );
			b->Answers[p][instance] = false;			// reinit
			Fence();
		} // if
		power += power;
	} // for

	if ( p == 0 ) {										// process 0 is always the champion
		b->BarrierCount = (b->BarrierCount + 1) % 2;
		b->Announcement[b->BarrierCount] = false;		// reinit
		b->Announcement[LocalBarrierCount] = true;		// all may proceed
		Fence();
	} else {											// not the champion
		await( b->Announcement[LocalBarrierCount] );
	} // if
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	TYPE cols = Clog2( N );

	b.Answers = Allocator( sizeof(typeof(b.Answers[0])) * N ); // rows
	b.Opponent = Allocator( sizeof(typeof(b.Opponent[0])) * N ); // rowa
	for ( TYPE i = 0; i < N; i += 1 ) {
		b.Answers[i] = Allocator( sizeof(typeof(b.Answers[0][0])) * cols ); // cols
		b.Opponent[i] = Allocator( sizeof(typeof(b.Opponent[0][0])) * cols ); // cols
	} // for

	TYPE power = 1;
	// must initialize cols first so that power has correct value
	for ( typeof(N) instance = 0; instance < cols; instance += 1 ) { // cols
		for ( typeof(N) process = 0; process < N; process += 1 ) { // rows
			b.Opponent[process][instance] = process ^ power;
			b.Answers[process][instance] = false;
		} // for
		power += power;
	} // for
	b.Announcement[0] = b.Announcement[1] = false;
	b.BarrierCount = 0;
} // ctor

void __attribute__((noinline)) dtor() {
	for ( typeof(N) instance = 0; instance < N; instance += 1 ) {
		free( (void *)b.Answers[instance] );
		free( (void *)b.Opponent[instance] );
	} // for
	free( b.Opponent );
	free( b.Answers );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierTour Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
