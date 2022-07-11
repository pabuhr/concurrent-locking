// Yih-Kuen Tsay, Deriving a Scalable Algorithm for Mutual Exclusion,
// Distributed Computing, Lecture Notes in Computer Science Volume 1499, 1998, pp 393-407 

enum Intent { DontWantIn, WantIn };
static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE intents[2] CALIGN = { DontWantIn, DontWantIn }, last CALIGN;
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

			intents[id] = WantIn;						// entry protocol
			last = id;									// RACE
			Fence();									// force store before more loads
			if ( FASTPATH( intents[other] != DontWantIn ) )	// local spin
				while ( last == id ) Pause();			// busy wait

			randomThreadChecksum += CS( id );

			intents[id] = DontWantIn;					// exit protocol
			last = id;

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
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Tsay Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
