// John M. Mellor-Crummey and Michael L. Scott, Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors,
// ACM Transactions on Computer Systems, 9(1), February 1991, Fig. 11, p. 40

typedef enum { winner, loser, bye, champion, dropout } Role;

typedef struct CALIGN {
	VTYPE * opponent;									// volatile to prevent warning
	VTYPE flag;
	Role role;
} rount_t;

typedef	struct {
	rount_t ** rounds;
} barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL TYPE sense = true;
#define BARRIER_CALL block( &b, p, &sense );

static inline void block( barrier * b, TYPE p, TYPE * sense ) {
	TYPE round = 1;

	// Special-case for N == 1 since it is assigned the "bye" role, which leads to an infinite loop in the first loop if
	// not skipped. This results because for N > 1 all threads have a champion, but for N == 1 there is champion.
	if ( N == 1 ) goto Loop1;
	
	for ( ;; ) {										// arrival
		switch ( b->rounds[p][round].role ) {
		  case loser:
			*b->rounds[p][round].opponent = *sense;
			await( b->rounds[p][round].flag == *sense ); // wait for champion to signal wakeup tree
			goto Loop1;
		  case winner:
			await( b->rounds[p][round].flag == *sense );
			break; // go to next round of tournament
		  case bye:										// do nothing
			break; // go to next round of tournament
		  case champion:
			await( b->rounds[p][round].flag == *sense );
			*b->rounds[p][round].opponent = *sense;
			goto Loop1;
		  case dropout:									// impossible
		  default:
			abort();
		} // switch
		round += 1;
	} // for
  Loop1: ;

	for ( ;; ) {										// wakeup
		round -= 1;
		switch( b->rounds[p][round].role ) {
		  case loser:									// impossible
			abort();
		  case winner:
			*b->rounds[p][round].opponent = *sense;
			break;
		  case bye:										// do nothing
			break;
		  case champion:								// impossible
			abort();
		  case dropout:
			goto Loop2;									// breaks when round == 0
		  default:
			abort();
		} // switch
	} // for
  Loop2: ;

	*sense = ! *sense;
} // block

//#define TESTING
#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	TYPE cols = Log2( N ) + 2;							// zero to Log2 inclusive
	b.rounds = Allocator( sizeof(typeof(b.rounds[0])) * N ); // rows

	// must initialize cols first so that opponent can be set properly
	for ( typeof(N) r = 0; r < N; r += 1 )
		b.rounds[r] = Allocator( sizeof(typeof(b.rounds[0][0])) * cols ); // cols

	TYPE pow2 = 1, pow2m1 = 0;	
	for ( typeof(N) c = 0; c < cols; c += 1 ) {
		for ( typeof(N) r = 0; r < N; r += 1 ) {
			b.rounds[r][c].flag = false;

			if        ( c > 0 && r % pow2 == 0 && r + pow2m1 < N && pow2 < N ) {
				b.rounds[r][c].role = winner;
				b.rounds[r][c].opponent = &b.rounds[r + pow2m1][c].flag;
			} else if ( c > 0 && r % pow2 == 0 && r + pow2m1 >= N ) {
				b.rounds[r][c].role = bye;
			} else if ( c > 0 && r % pow2 == pow2m1 ) {
				b.rounds[r][c].role = loser;
				b.rounds[r][c].opponent = &b.rounds[r - pow2m1][c].flag;
			} else if ( c > 0 && r == 0 && pow2 >= N ) {
				b.rounds[r][c].role = champion;
				b.rounds[r][c].opponent = &b.rounds[r + pow2m1][c].flag;
			} else if ( c == 0 ) {
				b.rounds[r][c].role = dropout;
			} else {
				// some matrix elements uninitialized
			}
		} // for
		pow2m1 = pow2;
		pow2 += pow2;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)b.rounds );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierTourMCS Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
