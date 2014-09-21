// Leslie Lamport, A Fast Mutual Exclusion Algorithm, ACM Transactions on Computer Systems, 5(1), 1987, Fig. 2, p. 5
// N => do not want in, versus 0 in original paper, so "b" is dimensioned 0..N-1 rather than 1..N.

#include <stdbool.h>

static volatile TYPE *b CALIGN;
static volatile TYPE x CALIGN, y CALIGN;
static TYPE PAD CALIGN __attribute__(( unused ));		// protect further false sharing

#define await( E ) while ( ! (E) ) Pause()

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
		  start: b[id] = true;							// entry protocol
			x = id;
			Fence();									// force store before more loads
			if ( FASTPATH( y != N ) ) {
				b[id] = false;
				Fence();								// force store before more loads
				await( y == N );
				goto start;
			} // if
			y = id;
			Fence();									// force store before more loads
			if ( FASTPATH( x != id ) ) {
				b[id] = false;
				Fence();								// force store before more loads
				for ( int j = 0; j < N; j += 1 )
					await( ! b[j] );
				if ( FASTPATH( y != id ) ) {
//					await( y == N );
					goto start;
				} // if
			} // if
			CriticalSection( id );
			y = N;										// exit protocol
			b[id] = false;
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			entry += 1;
		} // while
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

void ctor() {
	b = Allocator( sizeof(typeof(b[0])) * N );
	for ( int i = 0; i < N; i += 1 ) {					// initialize shared data
		b[i] = 0;
	} // for
	y = N;
} // ctor

void dtor() {
	free( (void *)b );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=LamportFast Harness.c -lpthread -lm" //
// End: //
