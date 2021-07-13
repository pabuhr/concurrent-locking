// Xiaodong Zhang, Yong Yan and Robert Castaneda, Evaluating and Designing Software Mutual Exclusion Algorithms on
// Shared-Memory Multiprocessors, Parallel Distributed Technology: Systems Applications, IEEE, 1996, 4(1), Figure 14,
// p. 37

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE ** x CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define min( x, y ) ((x) < (y) ? (x) : (y))
#define logx( N, b ) (log(N) / log(b))

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	int high, k, i, j, l, len;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		high = ceil( logx( N, Degree ) );				// maximal depth of binary tree
		j = 0;
		l = id;
		for ( entry = 0; stop == 0; entry += 1 ) {
			k = id / Degree;
			len = N;
			while ( j < high ) {
			  yy: x[j][l] = 1;
				Fence();								// force store before more loads
				for ( i = k * Degree; i < l; i += 1 ) {
					if ( x[j][i] ) {
						x[j][l] = 0;
						Fence();						// force store before more loads
						while ( x[j][i] != 0 ) Pause();
						goto yy;
					} // if
				} // for
				for ( i = l + 1; i < min((k + 1) * Degree, len); i += 1 )
					while ( x[j][i] ) Pause();
				l = l / Degree;
				k = k / Degree;
				j += 1;
				len = (len % Degree == 0) ? len / Degree : len / Degree + 1;
			} // for

			randomThreadChecksum += CriticalSection( id );

			//j = high;
			int pow2 = pow( Degree, high );
			while ( j > 0 ) {
				j -= 1;
				pow2 /= Degree;
				l = id / pow2;
				x[j][l] = 0;
			} // while
			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			l = id;
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
	if ( Degree == -1 ) {
		printf( "Usage: missing d-ary for tree node.\n" );
		exit( EXIT_FAILURE );
	} // if

	x = Allocator( sizeof(typeof(x[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		x[i] = Allocator( sizeof(typeof(x[0][0])) * N );
	} // for
	for ( typeof(N) i = 0; i < N; i += 1 ) {					// initialize shared data
		for ( typeof(N) j = 0; j < N; j += 1 ) {
			x[i][j] = 0;
		} // for
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		free( (void *)x[i] );
	} // for
	free( (void *)x );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=ZhangdT Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
