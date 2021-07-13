// Eugene Styer. Improving Fast Mutual Exclusion. Proceedings of the 2nd International Conference on Supercomputing,
// 1992, Figure 1, Page 162. Below is the incomplete implementation outline as given in the paper.  Our attempt to
// implement the algorithm found two independent errors both resulting in mutual exclusion violations.  It appears both
// Lemmas 3.1 and 3.2 have errors.

#include <stdbool.h>									// true, false

enum { Slow, Fast, Out };

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE * W CALIGN, ** S CALIGN, Turn CALIGN, Lock CALIGN, Block CALIGN, l CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

void CheckVars( int Lev, int Start ) {
	if ( Lev > 0 ) {
		for ( int j = 0; j < k - 1; j += 1 ) {
			while ( W[ S[Lev][Start + j] ] == Fast );
			if ( W[ S[Lev][Start + j]] == Slow )
				CheckVars( Lev - 1, (Start + j) * k );
			else
				for ( j = 0; j < k - 1; k += 1 )
					while ( W[ S[Lev][Start + j] ] == Fast );
		} // for
	} // if
} // CheckVars

int sub( int i, int j ) {
	return i / (k << j);
} // sub
		
static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	int j;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			W[id] = Fast;
			Turn = id;
			if ( ! Lock ) {
				for ( j = 1; j < l - 1; j += 1 )
					S[j][sub(id, j)] = id;
				Lock = true;
				if ( Turn == id && ! Block ) goto CS;
			}
			W[id] = Slow;
			for ( j = 0; j < l - 1; j += 1 )
				Entry( i, sub(id, j + 1), sub( id, j ) );
			Block = true;
			CheckVars( l - 1, 0 );
		  CS: ;

			randomThreadChecksum += CriticalSection( id );

			Lock = false;
			if ( W[id] ) {
				Block = false;
				W[id] = Out;
				for ( j = l - 1; j >= 0; j -= 1 )
					Exit( j, sub( i, j + 1 ), sub( i, j ) );
			} else
				W[id] = Out;

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		__sync_fetch_and_add( &sumOfThreadChecksums, randomThreadChecksum );

		#ifdef FAST
		id = oid;
		#endif // FAST
		entries[r][id] = entry;
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	W = Allocator( sizeof(typeof(W[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {					// initialize shared data
		W[i] = 0;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)W );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Styer Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
