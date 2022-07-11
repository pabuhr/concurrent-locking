// Leslie Lamport, The Mutual Exclusion Problem: Part II - Statement and Solutions, Journal of the ACM, 33(2), 1986,
// Fig. 1, p. 337

enum Intent { DontWantIn, WantIn };

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static volatile TYPE * intents CALIGN;					// shared
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

		  L: intents[id] = WantIn;
			Fence();									// force store before more loads
			for ( typeof(id) j = 0; j < id; j += 1 ) {	// check if thread with higher id wants in
			// for ( int j = id - 1; j >= 0; j -= 1 ) {	// search opposite direction
				if ( intents[j] == WantIn ) {
					intents[id] = DontWantIn;
					Fence();							// force store before more loads
					while ( intents[j] == WantIn ) Pause();
					goto L;
				} // if
			} // for
			for ( typeof(N) j = id + 1; j < N; j += 1 )
				while ( intents[j] == WantIn ) Pause();
			Fence();									// force store before more loads

			randomThreadChecksum += CS( id );

			Fence();									// force store before more loads
			intents[id] = DontWantIn;					// exit protocol

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
	intents = Allocator( sizeof(typeof(intents[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {			// initialize shared data
		intents[i] = DontWantIn;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)intents );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=LamportRetract Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
