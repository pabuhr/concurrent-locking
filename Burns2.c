// James E. Burns, Symmetry in Systems of Asynchronous Processes. 22nd Annual Symposium on Foundations of Computer
// Science, 1981, Figure 2, p 170. Some improvements have been made.

#include "FCFS.h"

#include <stdbool.h>

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE turn CALIGN, * flag CALIGN;
FCFSGlobal();
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	FCFSLocal();

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	typeof(N) j;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			FCFSEnter();
		  L0: ;
			flag[id] = true;							// entry protocol
			WO( Fence(); )								// write order matters
			turn = id;									// RACE
			Fence();									// prevent reading turn below before intent is set above
			if ( FASTPATH( turn != id ) ) {
			  L1: ;
				flag[id] = false;						// retract intent
				Fence();
			  L11: ;
				for ( j = 0; j < N; j += 1 ) {
					if ( FASTPATH( flag[j] ) ) { Pause(); goto L11; }
				} // for
				goto L0;								// rare path
			} else {
//				flag[id] = true;
//				Fence();								// force store before more loads
			  L2: ;
				if ( SLOWPATH( turn != id ) ) { goto L1; }
				for ( j = 0; j < N; j += 1 ) {
					if ( SLOWPATH( j != id && flag[j] ) ) { goto L2; }
				} // for
			} // if
			WO( Fence(); )
			FCFSExit();

			randomThreadChecksum += CS( id );

			WO( Fence(); );								// prevent write floating up
			flag[id] = false;							// exit protocol

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
	flag = Allocator( sizeof(typeof(flag[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {			// initialize shared data
		flag[i] = false;
	} // for
	FCFSCtor();
} // ctor

void __attribute__((noinline)) dtor() {
	FCFSDtor();
	free( (void *)flag );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Burns2 Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
