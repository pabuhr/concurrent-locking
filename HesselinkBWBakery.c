// Hesselink, W.H. Correctness and Concurrent Complexity of the Black-White Bakery Algorithm. Formal Aspects of
// Computing, Figure 1, 28, 325-341 (2016). https://doi-org.proxy.lib.uwaterloo.ca/10.1007/s00165-016-0364-4

#include <stdbool.h>

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE color CALIGN;
static VTYPE * cho CALIGN, * pair CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define fn( n, mcol ) (1 + (n % 2 == mcol ? n / 2 : 0))
#define guardA( n, mcol, lev, p, thr ) (n / 2 == 0 || n % 2 != mcol || lev < n / 2 || (p <= thr && lev <= n / 2))
#define guardB( n, mcol ) ((n / 2 == 0 || n % 2 == mcol))

#define max( x, y ) ((x) < (y) ? (y) : (x))

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	TYPE mcol, lev, v;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	typeof(&cho[0]) mycho = &cho[id];					// optimization
	typeof(&pair[0]) mypair = &pair[id];

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			*mycho = true;
			Fence();
			mcol = color;
			lev = 1; 
			for ( typeof(N) thr = 0; thr < N; thr += 1 ) {
				// Macros fn has multiple reads of "n", which must only be read once, so variable pair[thr] is copied
				// into local variable read "v" preventing multiple reads.
				if ( thr != id ) {
					v = pair[thr];
					lev = max( lev, fn( v, mcol ) );
				} // if
			} // for
			*mypair = 2 * lev + mcol;
			WO( Fence(); );
			*mycho = false;
			Fence();

			for ( typeof(N) thr = 0; thr < N; thr += 1 ) {
				if ( thr != id ) {
					typeof(&cho[0]) othercho = &cho[thr]; // optimization
					await( ! *othercho );
					WO( Fence(); );
					// Macros guardA and guardB have multiple reads of "n", but each read occurs in an independent
					// disjunction, where reading different values for each disjunction does not affect correctness.
					typeof(&pair[0]) otherpair = &pair[thr]; // optimization
					if ( *otherpair % 2 == mcol )
						await( guardA( *otherpair, mcol, lev, id, thr ) );
					else 
						await( guardB( *otherpair, mcol ) || color != mcol ); 
				} // if
			} // for
			WO( Fence(); );

			randomThreadChecksum += CS( id );

			WO( Fence(); );
			color = 1 - mcol;
			WO( Fence(); );
			*mypair = mcol;
			WO( Fence(); );

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			mycho = &cho[id];							// optimization
			mypair = &pair[id];
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
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=HesselinkBWBakery Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
