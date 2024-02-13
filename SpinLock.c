#include "SpinLock.h"

#include xstr(FCFS.h)									// include algorithm for testing

static TYPE PAD3 CALIGN __attribute__(( unused ));		// protect further false sharing
FCFSGlobal();
static TYPE PAD4 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
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
			spin_lock( &lock );
			FCFSExitAcq();

			randomThreadChecksum += CS( id );

			FCFSTestExit();
			spin_unlock( &lock );
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
		entries[r][id] = entry;
		Fai( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		Fai( &Arrived, -1 );
	} // for

	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	Clr( &lock );										// Fence
	FCFSCtor();

	if ( N == 1 ) {										// title
		#ifdef CFMT
		#ifndef NOEXPBACK
		printf( " EXPBACK(%d,%d)", SPIN_START, SPIN_END );
		#else
		printf( " NOEXPBACK" );
		#endif // ! NOEXPBACK
		#endif // CFMT
	} // if
} // ctor

void __attribute__((noinline)) dtor() {
	FCFSDtor();
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=SpinLock Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
