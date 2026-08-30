#include "BarrierCallback.h"

#if 0
static inline void done( Barrier * b, TYPE p, TYPE T ) {
	b->sleep[p] = true;
	TYPE ownX = Fai( b->nX, 1 );
	if ( ownX == 0 ) {									// quorum leader
		await( b->nX > (STYPE)T - b->group );
		await( b->nIn == (STYPE)T - b->nX );
		TYPE w = (b->nX < (STYPE)T ? b->group - (T - b->nX) : 0);
		if ( w > 0 ) b->poison = true;
		for ( size_t q = 0; q < T; q += 1 ) {
			if ( b->sleep[q] && w > 0 ) {
				b->halt[q] = 1; w -= 1;
			} else {
				b->halt[q] = 2;
			} // if
		} // for
	} // if
	await( b->halt[p] > 0 );
	if ( b->halt[p] == 1 ) {
		//BARRIER_CALL;
		block( b );
	} // if
	b->halt[p] = b->sleep[p] = 0;						// reset thread variables
	if ( ownX == 0 ) {
		for ( size_t q = 0; q < T; q += 1 ) {			// wait for quorum followers to exit barrier
			await( b->halt[q] == 0 );
		} // for
		b->nX = b->nIn = b->poison = 0;					// reset global variable
	} // if
} // done
#endif

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

		done( &b, p );										// all threads call

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
