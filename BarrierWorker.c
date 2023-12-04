static TYPE PAD3 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE CALIGN stopcnt = 0;
static VTYPE CALIGN * cnt;
static TYPE PAD4 CALIGN __attribute__(( unused ));		// protect further false sharing

#ifndef FREQ
#define FREQ 0xffff
#endif // AMP

enum { Frequency = FREQ };

static void * Worker( void * arg ) {
	TYPE p = (size_t)arg;
	TYPE totalentries = 0;

	NCS_DECL;

	BARRIER_DECL;

	for ( int r = 0; r < RUNS; r += 1 ) {
		// Stopping barriers for a time experiment is done using a monotonic counter that counts the total entries
		// through the barrier. This counter ensures all threads remain in lockstep through the barrier, i.e., no thread
		// barges ahead to a new cycle leaving other threads to complete the current cycle.
		do {
			NCS;

			// testing the barrier condition:
			if ( ((totalentries) & Frequency) == Frequency ) {
				if ( p == 0 ) {
					// cnt[p] and cnt[k] have been set in the previous NCS
					for ( TYPE k = 0 ; k < N ; k += 1 ) {
						if ( cnt[k] != cnt[p] ) {
							printf( "***ERROR*** barrier failure Id:%zu\n", p );
							abort();
						} // if
					} // for
				} // if
			} // if

			totalentries += 1;

			if ( (totalentries & Frequency) == Frequency ) { // to be implemented by masking
				cnt[p] = totalentries;
			} // if

			if ( p == 0 && stop ) {						// termination code
				// thread 0 has not yet entered the current barrier, therefore all other threads q have not yet passed
				// this barrier and cannot pass it before the next assignment to stopcnt, implying cnt[q] <= cnt[0].
				stopcnt = totalentries;
			} // if

			BARRIER_CALL;
		} while ( totalentries != stopcnt );

		entries[r][p] = totalentries;
		totalentries = 0;

		cnt[p] = 0;

		Fai( &Arrived, 1 );
		while ( stop ) Pause();
		if ( p == 0 ) stopcnt = 0;
		Fai( &Arrived, -1 );
	} // for

	return NULL;
} // Worker

void __attribute__((noinline)) worker_ctor() {
	#ifdef CFMT
	if ( N == 1 ) printf( " TESTING freq=%#x/%d", Frequency, Frequency );
	#endif // CFMT
	cnt = Allocator( sizeof(typeof(cnt[0])) * N );
} // worker_ctor

void __attribute__((noinline)) worker_dtor() {
	free( (void *)cnt );
} // worker_dtor
