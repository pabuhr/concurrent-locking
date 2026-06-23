// Eugene D. Brooks III, The Butterfly Barrier, International Journal of Parallel Programming, Vol. 15, No. 4, 1986, pp
// 295-307. See Appredix (pp. 305-307) for algorithm.

// Cannot have callback/distinguished-thread without changing from symmetric.

#ifndef ATOMIC
#define ATOMIC											// Too complex for hand fencing
#endif // ! ATOMIC

// Handshaking to prevent re-initialization problem.
#define UNLOCK( i, p ) { while ( b->locks[i][p] ); b->locks[i][p] = 1; }
#define LOCK( i, p ) { while ( ! b->locks[i][p] ); b->locks[i][p] = 0; }

// The WORK #(p) macro implements the work for an # level barrier.  The argument p is the thread number. The WORK #M(p)
// macro is the work section for a processor that has to perform substitutions.
#define WORK1( p )  UNLOCK( 0, p ^ (1 << 0) ); LOCK( 0, p );

#define WORK2( p )  UNLOCK( 0, p ^ (1 << 0) ); LOCK( 0, p ); \
					UNLOCK( 1, p ^ (1 << 1) ); LOCK( 1, p );
#define WORK2M( p ) UNLOCK( 0, (p ^ (4 - 1)) ^ (1 << 0) ); \
					UNLOCK( 0, p ^ (1 << 0) ); \
					LOCK( 0, p ); LOCK( 0, p ^ (4 - 1) ); \
					UNLOCK( 1, (p ^ (4 - 1)) ^ (1 << 1) ); \
					UNLOCK( 1, p ^ (1 << 1) ); \
					LOCK( 1, p ); LOCK( 1, p ^ (4 - 1) );

#define WORK3( p )  UNLOCK( 0, p ^ (1 << 0) ); LOCK( 0, p ); \
					UNLOCK( 1, p ^ (1 << 1) ); LOCK( 1, p ); \
					UNLOCK( 2, p ^ (1 << 2) ); LOCK( 2, p );
#define WORK3M( p ) UNLOCK( 0, (p ^ (8 - 1)) ^ (1 << 0) ); \
					UNLOCK( 0, (p ^ (1 << 0))); \
					LOCK( 0, p ); LOCK( 0, p ^ (8 - 1) ); \
					UNLOCK( 1, (p ^ (8 - 1)) ^ (1 << 1) ); \
					UNLOCK( 1, (p ^ (1 << 1) ) ); \
					LOCK( 1, p ); LOCK( 1, p ^ (8 - 1) ); \
					UNLOCK( 2, (p ^ (8 - 1)) ^ (1 << 2) ); \
					UNLOCK( 2, p ^ (1 << 2)); \
					LOCK( 2, p ); LOCK( 2, p ^ (8 - 1) );

#define WORK4( p )  UNLOCK( 0, p ^ (1 << 0) ); LOCK( 0, p ); \
					UNLOCK( 1, p ^ (1 << 1) ); LOCK( 1, p ); \
					UNLOCK( 2, p ^ (1 << 2) ); LOCK( 2, p ); \
					UNLOCK( 3, p ^ (1 << 3) ); LOCK( 3, p );
#define WORK4M( p ) UNLOCK( 0, (p ^ (16 - 1)) ^ (1 << 0) ); \
					UNLOCK( 0, (p ^ (1 << 0))); \
					LOCK( 0, p ); LOCK( 0, p ^ (16 - 1) ); \
					UNLOCK( 1, (p ^ (16 - 1)) ^ (1 << 1) ); \
					UNLOCK( 1, (p ^ (1 << 1) ) ); \
					LOCK( 1, p ); LOCK( 1, p ^ (16 - 1) ); \
					UNLOCK( 2, (p ^ (16 - 1)) ^ (1 << 2) ); \
					UNLOCK( 2, p ^ (1 << 2)); \
					LOCK( 2, p ); LOCK( 2, p ^ (16 - 1) ); \
					UNLOCK( 3, (p ^ (16 - 1)) ^ (1 << 3) ); \
					UNLOCK( 3, p ^ (1 << 3)); \
					LOCK( 3, p ); LOCK( 3, p ^ (16 - 1) );

#define WORK5( p )  UNLOCK( 0, p ^ (1 << 0) ); LOCK( 0, p ); \
					UNLOCK( 1, p ^ (1 << 1) ); LOCK( 1, p ); \
					UNLOCK( 2, p ^ (1 << 2) ); LOCK( 2, p ); \
					UNLOCK( 3, p ^ (1 << 3) ); LOCK( 3, p ); \
					UNLOCK( 4, p ^ (1 << 4) ); LOCK( 4, p );
#define WORK5M( p ) UNLOCK( 0, (p ^ (32 - 1)) ^ (1 << 0) ); \
					UNLOCK( 0, (p ^ (1 << 0))); \
					LOCK( 0, p ); LOCK( 0, p ^ (32 - 1) ); \
					UNLOCK( 1, (p ^ (32 - 1)) ^ (1 << 1) ); \
					UNLOCK( 1, (p ^ (1 << 1) ) ); \
					LOCK( 1, p ); LOCK( 1, p ^ (32 - 1) ); \
					UNLOCK( 2, (p ^ (32 - 1)) ^ (1 << 2) ); \
					UNLOCK( 2, p ^ (1 << 2)); \
					LOCK( 2, p ); LOCK( 2, p ^ (32 - 1) ); \
					UNLOCK( 3, (p ^ (32 - 1)) ^ (1 << 3) ); \
					UNLOCK( 3, p ^ (1 << 3)); \
					LOCK( 3, p ); LOCK( 3, p ^ (32 - 1) ); \
					UNLOCK( 4, (p ^ (32 - 1)) ^ (1 << 4) ); \
					UNLOCK( 4, p ^ (1 << 4)); \
					LOCK( 4, p ); LOCK( 4, p ^ (32 - 1) );

#define WORK6( p )  UNLOCK( 0, p ^ (1 << 0) ); LOCK( 0, p ); \
					UNLOCK( 1, p ^ (1 << 1) ); LOCK( 1, p ); \
					UNLOCK( 2, p ^ (1 << 2) ); LOCK( 2, p ); \
					UNLOCK( 3, p ^ (1 << 3) ); LOCK( 3, p ); \
					UNLOCK( 4, p ^ (1 << 4) ); LOCK( 4, p ); \
					UNLOCK( 5, p ^ (1 << 5) ); LOCK( 5, p );
#define WORK6M( p ) UNLOCK( 0, (p ^ (64 - 1)) ^ (1 << 0) ); \
					UNLOCK( 0, (p ^ (1 << 0))); \
					LOCK( 0, p ); LOCK( 0, p ^ (64 - 1) ); \
					UNLOCK( 1, (p ^ (64 - 1)) ^ (1 << 1) ); \
					UNLOCK( 1, (p ^ (1 << 1) ) ); \
					LOCK( 1, p ); LOCK( 1, p ^ (64 - 1) ); \
					UNLOCK( 2, (p ^ (64 - 1)) ^ (1 << 2) ); \
					UNLOCK( 2, p ^ (1 << 2)); \
					LOCK( 2, p ); LOCK( 2, p ^ (64 - 1) ); \
					UNLOCK( 3, (p ^ (64 - 1)) ^ (1 << 3) ); \
					UNLOCK( 3, p ^ (1 << 3)); \
					LOCK( 3, p ); LOCK( 3, p ^ (64 - 1) ); \
					UNLOCK( 4, (p ^ (64 - 1)) ^ (1 << 4) ); \
					UNLOCK( 4, p ^ (1 << 4)); \
					LOCK( 4, p ); LOCK( 4, p ^ (64 - 1) ); \
					UNLOCK( 5, (p ^ (64 - 1)) ^ (1 << 5) ); \
					UNLOCK( 5, p ^ (1 << 5)); \
					LOCK( 5, p ); LOCK( 5, p ^ (64 - 1) );

#define WORK7( p )  UNLOCK( 0, p ^ (1 << 0) ); LOCK( 0, p ); \
					UNLOCK( 1, p ^ (1 << 1) ); LOCK( 1, p ); \
					UNLOCK( 2, p ^ (1 << 2) ); LOCK( 2, p ); \
					UNLOCK( 3, p ^ (1 << 3) ); LOCK( 3, p ); \
					UNLOCK( 4, p ^ (1 << 4) ); LOCK( 4, p ); \
					UNLOCK( 5, p ^ (1 << 5) ); LOCK( 5, p ); \
					UNLOCK( 6, p ^ (1 << 6) ); LOCK( 6, p );
#define WORK7M( p ) UNLOCK( 0, (p ^ (128 - 1)) ^ (1 << 0) ); \
					UNLOCK( 0, (p ^ (1 << 0))); \
					LOCK( 0, p ); LOCK( 0, p ^ (128 - 1) ); \
					UNLOCK( 1, (p ^ (128 - 1)) ^ (1 << 1) ); \
					UNLOCK( 1, (p ^ (1 << 1) ) ); \
					LOCK( 1, p ); LOCK( 1, p ^ (128 - 1) ); \
					UNLOCK( 2, (p ^ (128 - 1)) ^ (1 << 2) ); \
					UNLOCK( 2, p ^ (1 << 2)); \
					LOCK( 2, p ); LOCK( 2, p ^ (128 - 1) ); \
					UNLOCK( 3, (p ^ (128 - 1)) ^ (1 << 3) ); \
					UNLOCK( 3, p ^ (1 << 3)); \
					LOCK( 3, p ); LOCK( 3, p ^ (128 - 1) ); \
					UNLOCK( 4, (p ^ (128 - 1)) ^ (1 << 4) ); \
					UNLOCK( 4, p ^ (1 << 4)); \
					LOCK( 4, p ); LOCK( 4, p ^ (128 - 1) ); \
					UNLOCK( 5, (p ^ (128 - 1)) ^ (1 << 5) ); \
					UNLOCK( 5, p ^ (1 << 5)); \
					LOCK( 5, p ); LOCK( 5, p ^ (128 - 1) ); \
					UNLOCK( 6, (p ^ (128 - 1)) ^ (1 << 6) ); \
					UNLOCK( 6, p ^ (1 << 6)); \
					LOCK( 6, p ); LOCK( 6, p ^ (128 - 1) );

#define LNMAXPROC 7 /* 2^7 == 128 maximum threads */
#define MAXPROC (1 << LNMAXPROC)

typedef struct {
	TYPE CALIGN group;
	VTYPE CALIGN locks[LNMAXPROC][MAXPROC];
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b, p );

static inline void block( Barrier * b, TYPE p ) {
	switch( b->group ) {
	  case 1:
		break;
	  case 2:
		WORK1( p );
		break;
	  case 3 ... 4:
		if ( p < 4 - b->group ) {
			WORK2M( p );
		} else {
			WORK2( p );
		}
		break;
	  case 5 ... 8:
		if ( p < 8 - b->group ) {
			WORK3M( p );
		} else {
			WORK3( p );
		}
		break;
	  case 9 ... 16:
		if ( p < 16 - b->group ) {
			WORK4M( p );
		} else {
			WORK4( p );
		}
		break;
	  case 17 ... 32:
		if ( p < 32 - b->group ) {
			WORK5M( p );
		} else {
			WORK5( p );
		}
		break;
	  case 33 ... 64:
		if ( p < 64 - b->group ) {
			WORK6M( p );
		} else {
			WORK6( p );
		}
		break;
	  case 65 ... 128:
		if ( p < 128 - b->group ) {
			WORK7M( p );
		} else {
			WORK7( p );
		}
		break;
	  default:
		fprintf( stderr, "*** Error *** unsupported processor %zd\n", p );
		abort();
	} // switch
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b.group = N;
	for ( typeof(N) r = 0; r < LNMAXPROC; r += 1 ) {
		for ( typeof(N) c = 0; c < MAXPROC; c += 1 ) {
			b.locks[r][c] = 0;
		} // for
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierButterfly Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
