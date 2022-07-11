// Aravind, Simple, Space-Efficient, and Fairness Improved FCFS Mutual Exclusion Algorithms, J. Parallel
// Distrib. Comput. 73 (2013), Fig. 3, p. 1033.
// Moved turn[id] = 0; after the critical section for performance reasons.

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE * D CALIGN, * T CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	TYPE S[N];
	typeof(N) j, t;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;
		t = 1;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			D[id] = 1;									// phase 1, FCFS
			Fence();									// force store before more loads
			for ( j = 0; j < N; j += 1 )				// copy turn values
				S[j] = T[j];
			T[id] = t;									// advance turn
			D[id] = 0;
			Fence();									// force store before more loads
			for ( j = 0; j < N; j += 1 )
				if ( S[j] != 0 )						// want in ?
					while ( S[j] == T[j] ) Pause();
		  L: D[id] = 1;									// phase 2, B-L entry protocol, stage 1
			Fence();									// force store before more loads
			for ( j = 0; j < id; j += 1 )
				if ( D[j] != 0 ) {
					D[id] = 0;
					Fence();							// force store before more loads
					while ( D[j] != 0 ) Pause();
					goto L;								// restart
				} // if
			for ( j = id + 1; j < N; j += 1 )			// B-L entry protocol, stage 2
				while ( D[j] != 0 ) Pause();
			// T[id] = 0;								// original position

			randomThreadChecksum += CS( id );

			D[id] = 0;									// B-L exit protocol
			T[id] = 0;									// new position
			t = t < 3 ? t + 1 : 1;						// [1..3]

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
	D = Allocator( sizeof(typeof(D[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		D[i] = 0;
	} // for
	T = Allocator( sizeof(typeof(T[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		T[i] = 0;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)T );
	free( (void *)D );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=AravindF3 Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
