// James H. Anderson, Yong-Jik Kim, and Ted Herman. Shared-memory Mutual Exclusion: Major Research Trends Since 1986,
// Distributed Computing, 16 2003, Fig 2, p 78.
//
// Note the original version of this algorithm:
//
//   Thomas E. Anderson, The Performance of Spin Lock Alternatives for Shared-Memory Multiprocessors, IEEE Transactions
//   on Parallel and Distributed Systems, (1)1, Jan 1990, Table V, p. 12.
//
// fails when queueLast overflows back to 0 and N mod (maxint + 1) != 0.
//
// For example, assume a 3 bit number (values 0-7), N = 3, and threads interleave execution:
//
//   queueLast: 0, 1, 2, 3, 4, 5, 6, 7, 0 (reset to 0)
//   myPlace1 : 0,       0,       0,    
//   myPlace2 :    1,       1,       1, 
//   myPlace3 :       2,       2,       0
//
// Threads 1 and 3 now have the same ticket.
//
// Michael Scott suggests moving "flags[myPlace].v = MUST_WAIT" before the cricital section:
//
//    https://www.cs.rochester.edu/research/synchronization/pseudocode/ss.html#anderson
//
// but performance seems slower.

enum { MUST_WAIT = 0, HAS_LOCK = 1 };

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE * flags CALIGN;							// shared
static TYPE queueLast CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	TYPE myPlace;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			myPlace = Fai( queueLast, 1 );
			// Restore queueLast to [0,N) on roll over, but modified Fig 2 to remove modulus in the fast path by adding
			// an extra atomic instruction every Nth entry.
			if ( UNLIKELY( myPlace >= N ) ) {
				if ( myPlace == N ) Fai( queueLast, -N );
				myPlace -= N;
			} // if
			while ( flags[myPlace] == MUST_WAIT ) Pause(); // busy wait
			WO( Fence(); );

			randomThreadChecksum += CS( id );

			WO( Fence(); );
			flags[myPlace] = MUST_WAIT;					// exit protocol, myPlace must be <= N
			WO( Fence(); );
			flags[cycleUp( myPlace, N )] = HAS_LOCK;	// cycleUp has no modulus

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
	flags = Allocator( sizeof(typeof(flags[0])) * N );
	flags[0] = HAS_LOCK;								// initialize shared data
	for ( typeof(N) i = 1; i < N; i += 1 ) {
		flags[i] = MUST_WAIT;
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
