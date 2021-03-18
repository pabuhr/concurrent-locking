// Aravind, Simple, Space-Efficient, and Fairness Improved FCFS Mutual Exclusion Algorithms, J. Parallel
// Distrib. Comput. 73 (2013), Fig. 4, p. 1035.

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static volatile TYPE * D CALIGN, * X CALIGN, ** T CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	TYPE S[N][2];
	typeof(N) j, bit;

	for ( int r = 0; r < RUNS; r += 1 ) {
		uint32_t randomThreadChecksum = 0;
		bit = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			D[id] = 1;
			Fence();									// force store before more loads
			for ( j = 0; j < N; j += 1 ) {
				S[j][0] = T[j][0];
				S[j][1] = T[j][1];
			} // for
			T[id][bit] = 1;
			D[id] = 0;
			Fence();									// force store before more loads
			for ( j = 0; j < N; j += 1 )
				if ( S[j][0] != 0 || S[j][1] != 0 )
					while ( S[j][0] == T[j][0] && S[j][1] == T[j][1] ) Pause();
		  L: X[id] = 1;
			Fence();									// force store before more loads
			for ( j = 0; j < id; j += 1 ) {
				if ( X[j] != 0 ) {
					X[id] = 0;
					Fence();							// force store before more loads
					while ( X[j] != 0 ) Pause();
					goto L;								// restart
				} // if
			} // for
			for ( j = id + 1; j < N; j += 1 )			// B-L entry protocol, stage 2
				while ( X[j] != 0 ) Pause();
			Fence();									// force store before more loads
			T[id][bit] = 0;
			Fence();									// force store before more loads
			for ( j = 0; j < N; j += 1 )
				while ( D[j] != 0 ) Pause();

			randomThreadChecksum += CriticalSection( id );

			X[id] = 0;
			bit = ! bit;

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
	D = Allocator( sizeof(typeof(D[0])) * N );
	X = Allocator( sizeof(typeof(X[0])) * N );
	T = Allocator( sizeof(typeof(T[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		T[i] = Allocator( sizeof(typeof(T[0][0])) * 2 );
	} // for
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		D[i] = X[i] = 0;
		T[i][0] = T[i][1] = 0;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		free( (void *)T[i] );
	} // for
	free( (void *)T );
	free( (void *)X );
	free( (void *)D );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=AravindF4 Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
