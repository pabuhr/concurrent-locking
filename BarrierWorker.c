#include "BarrierCallback.h"

static TYPE PAD3 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE CALIGN stopecnt = 0;
static VTYPE CALIGN * ecnt;
static TYPE PAD4 CALIGN __attribute__(( unused ));		// protect further false sharing

#ifndef FREQ
#define FREQ 0xffff
//#define FREQ 0x1
#endif // FREQ

enum { Frequency = FREQ };

static void * Worker( void * arg ) {
	TYPE p = (size_t)arg;
	TYPE totalentries = 0;

	NCS_DECL;

	BARRIER_DECL;

	for ( int r = 0; r < RUNS; r += 1 ) {				// repeat experiment N times to obtain average throughput
		// Stopping barriers for a time experiment is done using a monotonic counter that counts the total entries
		// through the barrier. This counter ensures all threads remain in lockstep through the barrier, i.e., no thread
		// barges ahead to a new cycle leaving other threads to complete the current cycle.
		do {
			NCS;										// NCS can be empty because the barrier test provides a random delay

			// checking the barrier condition
			if ( ((totalentries) & Frequency) == Frequency ) {
				// ecnt[p] and ecnt[k] have been set in the previous NCS
				for ( TYPE k = 0 ; k < N ; k += 1 ) {
					if ( ! (ecnt[p] <= ecnt[k] ) ) {	// assert
						fprintf( stderr, "***ERROR*** barrier failure Id:%zu ecnt[%zd] = %zd ecnt[%zd] = %zd\n",
								#if defined( __cplusplus ) && defined( ATOMIC )
								p, k, ecnt[k].load(), p, ecnt[p].load()
								#else
								p, k, ecnt[k], p, ecnt[p]
								#endif // __cplusplus
						);
						abort();
					} // if
				} // for
			} // if

			totalentries += 1;

			if ( (totalentries & Frequency) == Frequency ) { // to be implemented by masking
				ecnt[p] = totalentries;
			} // if

			if ( p == 0 && stop ) {						// termination code
				// thread 0 has not yet entered the current barrier, therefore all other threads q have not yet passed
				// this barrier and cannot pass it before the next assignment to stopecnt, implying ecnt[q] <= ecnt[0].
				stopecnt = totalentries;
			} // if

			BARRIER_CALL;
		} while ( totalentries != stopecnt );			// complete last barrier event, which may have started

		entries[r][p] = totalentries / N;				// convert to epochs
		totalentries = 0;

		ecnt[p] = 0;

		Fai( Arrived, 1 );
		while ( stop ) Pause();
		if ( p == 0 ) stopecnt = 0;
		Fai( Arrived, -1 );
	} // for

	return NULL;
} // Worker

void __attribute__((noinline)) worker_ctor() {
	#ifdef CFMT
	if ( N == 1 ) {
		printf( " BARRIER CHECKING freq=%#x/%d", Frequency, Frequency );
		CB( printf( ", CALLBACK" ); )
		CBCHK( printf( ", CALLBACK CHECK" ); )
	} // if
	#endif // CFMT
	ecnt = (TYPEOF(ecnt))Allocator( sizeof(TYPEOF(ecnt[0])) * N );
	for ( TYPEOF(N) i = 0; i < N; i += 1 ) ecnt[i] = 0;
} // worker_ctor

void __attribute__((noinline)) worker_dtor() {
	free( (void *)ecnt );
} // worker_dtor
