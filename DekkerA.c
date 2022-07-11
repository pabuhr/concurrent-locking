#ifdef FAST
// Want same meaning for FASTPATH for both experiments.
#undef FASTPATH
#define FASTPATH(x) __builtin_expect(!!(x), 1)
#endif // FAST

enum Intent { DontWantIn, WantIn };
static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE intents[2] CALIGN = { DontWantIn, DontWantIn }, last CALIGN = 0;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define inv( c ) ((c) ^ 1)

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	int other = inv( id );								// int is better than TYPE

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			for ( ;; ) {
				#ifdef FLICKER
				for ( int i = 0; i < 100; i += 1 ) intents[id] = i % 2; // flicker
				#endif // FLICKER

				intents[id] = WantIn;					// declare intent
				// Necessary to prevent the read of intents[other] from floating above the assignment
				// intents[id] = WantIn, when the hardware determines the two subscripts are different.
				Fence();								// force store before more loads
			  if ( FASTPATH( intents[other] == DontWantIn ) ) break;
				if ( last == id ) {
					#ifdef FLICKER
					for ( int i = 0; i < 100; i += 1 ) intents[id] = i % 2; // flicker
					#endif // FLICKER

					intents[id] = DontWantIn;
					// Optional fence to prevent LD of "last" from being lifted above store of
					// intends[id]=DontWantIn. Because a thread only writes its own id into "last",
					// and because of eventual consistency (writes eventually become visible),
					// the fence is conservative.
					//Fence();							// force store before more loads
					await( last != id );				// low priority busy wait
				} // if
			} // for

			randomThreadChecksum += CS( id );

			#ifdef FLICKER
			for ( int i = id; i < 100; i += 1 ) last = i % 2; // flicker
			#endif // FLICKER

			last = id;									// exit protocol

			#ifdef FLICKER
			for ( int i = 0; i < 100; i += 1 ) intents[id] = i % 2; // flicker
			#endif // FLICKER

			intents[id] = DontWantIn;

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			other = inv( id );
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		Fai( &sumOfThreadChecksums, randomThreadChecksum );

		#ifdef FAST
		id = oid;
		other = inv( id );
		#endif // FAST
		entries[r][id] = entry;
		Fai( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		Fai( &Arrived, -1 );
	} // for
	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	if ( N != 2 ) {
		printf( "\nUsage: N=%zd must be 2\n", N );
		exit( EXIT_FAILURE);
	} // if
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=DekkerA Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
