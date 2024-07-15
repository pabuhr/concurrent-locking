// Wim H. Hesselink, Verifying a Simplification of Mutual Exclusion by Lycklama-Hadzilacos, Acta Informatica, 2013,
// 50(3), Fig.4, p. 11

enum { R = 3 };

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE * intents CALIGN, * turn CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	typeof(N) Range = N * R;
	TYPE copy[Range];
	typeof(N) j, nx;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;
		nx = 0;
		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			intents[id] = 1;							// phase 1, FCFS
			Fence();									// force store before more loads
			for ( j = 0; j < Range; j += 1 )			// copy turn values
				copy[j] = turn[j];
			turn[id * R + nx] = 1;						// advance turn
			intents[id] = 0;
			Fence();									// force store before more loads
			for ( j = 0; j < Range; j += 1 )
				if ( copy[j] != 0 ) {					// want in ?
					while ( turn[j] != 0 ) Pause();
//					copy[j] = 0;
				} // if
		  L: intents[id] = 1;							// phase 2, B-L entry protocol, stage 1
			Fence();									// force store before more loads
			for ( j = 0; j < id; j += 1 )
				if ( intents[j] != 0 ) {
					intents[id] = 0;
					Fence();							// force store before more loads
					while ( intents[j] != 0 ) Pause();
					goto L;
				} // if
			for ( j = id + 1; j < N; j += 1 )			// B-L entry protocol, stage 2
				while ( intents[j] != 0 ) Pause();

			randomThreadChecksum += CS( id );

			intents[id] = 0;							// B-L exit protocol
			turn[id * R + nx] = 0;
			nx = cycleUp( nx, R );

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		Fai( sumOfThreadChecksums, randomThreadChecksum );

		#ifdef FAST
		id = oid;
		#endif // FAST
		entries[r][id] = entry;
		Fai( Arrived, 1 );
		while ( stop != 0 ) Pause();
		Fai( Arrived, -1 );
	} // for
	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	intents = Allocator( sizeof(typeof(intents[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {
		intents[i] = 0;
	} // for
	turn = Allocator( sizeof(typeof(turn[0])) * N * R );
	for ( typeof(N) i = 0; i < N * R; i += 1 ) {
		turn[i] = 0;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)turn );
	free( (void *)intents );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Hesselink Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
