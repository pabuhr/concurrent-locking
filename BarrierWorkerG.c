#include "BarrierCallback.h"

static void * Worker( void * arg ) {
	TYPE p = (size_t)arg;
	TYPE totalentries = 0;

	NCS_DECL;

	BARRIER_DECL;

	for ( int r = 0; r < RUNS; r += 1 ) {				// repeat experiment N times to obtain average throughput
		while ( ! stop ) {
			NCS;										// NCS can be empty because the barrier test provides a random delay

			totalentries += 1;							// all threads count barrier entries

			BarrierRet ret = BARRIER_CALL;
			if ( ret == QuorumFail ) { /* printf( "Q" ); */ }
		} // while

		done( &b );										// all threads call

		entries[r][p] = totalentries / N;				// convert from entry counts to epochs per thread
		totalentries = 0;

		Fai( Arrived, 1 );
		while ( stop ) Pause();
		Fai( Arrived, -1 );
	} // for

	return NULL;
} // Worker

void __attribute__((noinline)) worker_ctor() {
	#ifdef CFMT
	if ( N == 1 ) {
		CB( printf( ", CALLBACK" ); )
		CBCHK( printf( ", CALLBACK CHECK" ); )
	} // if
	#endif // CFMT
} // worker_ctor

void __attribute__((noinline)) worker_dtor() {
} // worker_dtor
