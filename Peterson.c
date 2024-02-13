// G. L. Peterson, Myths About the Mutual Exclusion Problem, Information Processing Letters, 1981, 12(3), Fig. 3, p. 116
// cnt is used to prove threads do not move evenly through levels.

#include xstr(FCFS.h)									// include algorithm for testing

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE * Q CALIGN, * turns CALIGN;
FCFSGlobal();
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg + 1;							// id 0 => don't-want-in
	uint64_t entry;

	FCFSLocal();

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			FCFSEnter();
			for ( TYPE rd = 1; rd < N; rd += 1 ) {		// entry protocol, round
				Q[id] = rd;								// current round
				WO( Fence(); )							// write order matters
				turns[rd] = id;							// RACE
				Fence();								// force store before more loads
			  L: for ( typeof(N) k = 1; k <= N; k += 1 ) { // find loser
					if ( k != id && Q[k] >= rd && turns[rd] == id ) { Pause(); goto L; }
				} // for
			} // for
			WO( Fence(); )
			FCFSExitAcq();

			randomThreadChecksum += CS( id );

			FCFSTestExit();
			WO( Fence(); );								// prevent write floating up
			Q[id] = 0;									// exit protocol
			FCFSExitRel();

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		Fai( &sumOfThreadChecksums, randomThreadChecksum );

		#ifdef FAST
		id = oid;
		#endif // FAST
		entries[r][id - 1] = entry;						// adjust for id + 1
		Fai( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		Fai( &Arrived, -1 );
	} // for

	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	Q = Allocator( sizeof(typeof(Q[0])) * (N + 1) );
	turns = Allocator( sizeof(typeof(turns[0])) * (N - 1 + 1) );
	for ( typeof(N) i = 1; i <= N; i += 1 ) {			// initialize shared data
		Q[i] = 0;
	} // for
	FCFSCtor();
} // ctor

void __attribute__((noinline)) dtor() {
	FCFSDtor();
	free( (void *)turns );
	free( (void *)Q );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Peterson Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
