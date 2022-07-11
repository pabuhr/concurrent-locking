// Joep L. W. Kessels, Arbitration Without Common Modifiable Variables, Acta Informatica, 17(2), 1982, pp. 140-141

#define inv( c ) ( (c) ^ 1 )

#include "Binary.c"

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Token * t CALIGN;
static TYPE PAD CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	unsigned int n, e[N];

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			n = N + id;
			while ( n > 1 ) {							// entry protocol
#if 1
//				int lr = n % 2;
				int lr = n & 1;
//				n = n / 2;
				n >>= 1;
				binary_prologue( lr, &t[n] );
				e[n] = lr;
#else
				binary_prologue( n % 2, &t[n / 2] );
				e[n / 2] = n % 2;
				n = n / 2;
#endif
			} // while

			randomThreadChecksum += CS( id );

			for ( n = 1; n < N; n = n + n + e[n] ) {	// exit protocol
//				n = n + n + e[n];
//				binary_epilogue( n & 1, &t[n >> 1] );
				binary_epilogue( e[n], &t[n] );
			} // for

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
	// element 0 not used
	t = Allocator( sizeof(typeof(t[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		t[i].Q[0] = t[i].Q[1] = 0;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)t );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Kessels Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
