// Edsger W. Dijkstra. Cooperating Sequential Processes. Technical Report,
// Technological University, Eindhoven, Netherlands 1965, pp. 58-59.

enum Intent { DontWantIn, WantIn };
static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE cc[2] CALIGN = { DontWantIn, DontWantIn }, turn CALIGN = 0;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define inv( c ) ((c) ^ 1)

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	int other = inv( id );								// int is better than TYPE

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
		  A1: cc[id] = WantIn;
			Fence();
		  L1: if ( FASTPATH( cc[other] == WantIn ) ) {
				if ( turn != id ) { Pause(); goto L1; }
				cc[id] = DontWantIn;
			  B1: if ( turn == id ) { Pause(); goto B1; }
				goto A1;
			}

			randomThreadChecksum += CriticalSection( id );

			turn = id;
			cc[id] = DontWantIn;

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
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=DekkerOrig Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
