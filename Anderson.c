// Thomas E. Anderson, The Performance of Spin Lock Alternatives for Shared-Memoiy Multiprocessors, IEEE Transactions on
// Parallel and Distributed Systems, (1)1, Jan 1990, Table V, p. 12.
// Modified to remove excessive use of modulus.

enum { MUST_WAIT = 0, HAS_LOCK = 1 };
typedef struct {
	VTYPE v;
} Flag CALIGN;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Flag * flags CALIGN;								// shared
static TYPE queueLast CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	TYPE myPlace;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			#ifndef OPT1
			myPlace = __sync_fetch_and_add( &queueLast, 1 ) % N;
			#else
			myPlace = __sync_fetch_and_add( &queueLast, 1 );
			// modified, restore queueLast to [0,N) on roll over => no modulus but an extra atomic instruction every N
			// times.
			if ( myPlace >= N ) {
				if ( myPlace == N ) __sync_fetch_and_add( &queueLast, -N );
				myPlace -= N;
			} // if
			#endif // ! OPT1
			WO( Fence(); );
			while ( flags[myPlace].v == MUST_WAIT ) Pause(); // busy wait
			WO( Fence(); );

			randomThreadChecksum += CriticalSection( id );

			WO( Fence(); );
			flags[myPlace].v = MUST_WAIT;				// exit protocol, myPlace must be <= N
			WO( Fence(); );
			flags[cycleUp( myPlace, N )].v = HAS_LOCK;	// cycleUp has no modulus

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
	flags = Allocator( sizeof(typeof(flags[0])) * N );
	flags[0].v = HAS_LOCK;								// initialize shared data
	for ( typeof(N) i = 1; i < N; i += 1 ) {
		flags[i].v = MUST_WAIT;
	} // for
	queueLast = 0;
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)flags );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Anderson Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
