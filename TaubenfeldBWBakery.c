// Hesselink, W.H. Correctness and Concurrent Complexity of the Black-White Bakery Algorithm. Formal Aspects of
// Computing, Figure 1, 28, 325-341 (2016). https://doi-org.proxy.lib.uwaterloo.ca/10.1007/s00165-016-0364-4

#include <stdbool.h>

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
volatile TYPE color CALIGN;
volatile TYPE * cho CALIGN, * pair CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define fn(n, mcol) (1 + (n % 2 == mcol ? n / 2 : 0))
#define guardA(n, mcol, lev, p, thr) (n / 2 == 0 || n % 2 != mcol || (lev <= n / 2 && (lev < n / 2 || p <= thr))) /* where <= is the lexical order */
#define guardB(n, mcol) ((n / 2 == 0 || n % 2 == mcol))

#define max( x, y ) ((x) < (y) ? (y) : (x))
#define await( E ) while ( ! (E) ) Pause()

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	TYPE mcol;
	TYPE lev;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		for ( entry = 0; stop == 0; entry += 1 ) {
			cho[id] = true;
			mcol = color; lev = 1; 
			Fence();
			for ( typeof(N) thr = 0; thr < N; thr += 1 ) {
				if ( thr != id ) lev = max( lev, fn( pair[thr], mcol ) );
			}
			pair[id] = 2 * lev + mcol;
			cho[id] = false;
			Fence();
			for ( typeof(N) thr = 0; thr < N; thr += 1 ) {
				if ( thr != id ) {
					await( ! cho[thr] );
					if ( pair[thr] % 2 == mcol )
						await( guardA( pair[thr], mcol, lev, id, thr ) );
					else 
						await( guardB( pair[thr], mcol ) || color != mcol ); 
				}  // if
			} // for

			CriticalSection( id );

			color = 1 - mcol;
			pair[id] = mcol;

#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
		} // for

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
	color = 0;
	cho = Allocator( sizeof(typeof(cho[0])) * N );
	pair = Allocator( sizeof(typeof(pair[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {			// initialize shared data
		cho[i] = false;
		pair[i] = 0;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)pair );
	free( (void *)cho );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=TaubenfeldBWBakery Harness.c -lpthread -lm -DCFMT -DCNT=0" //
// End: //
