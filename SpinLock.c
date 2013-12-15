volatile TYPE lock CALIGN;

void spin_lock( volatile TYPE *lock ) {
	enum { SPIN_START = 4, SPIN_END = 64 * 1024, };
	unsigned int spin = SPIN_START;

	for ( ;; ) {
	  if ( *lock == 0 && __sync_lock_test_and_set( lock, 1 ) == 0 ) break;
		for ( int i = 0; i < spin; i += 1 ) Pause();	// exponential spin
		spin += spin;									// powers of 2
		if ( spin > SPIN_END ) spin = SPIN_START;		// prevent overflow
	} // for
} // spin_lock

void spin_unlock( volatile TYPE *lock ) {
	__sync_lock_release( lock );
} // spin_unlock

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
#ifdef FAST
	unsigned int cnt = 0;
#endif // FAST
	size_t entries[RUNS];

	for ( int r = 0; r < RUNS; r += 1 ) {
		entries[r] = 0;
		while ( stop == 0 ) {
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			spin_lock( &lock );
			CriticalSection( id );
			spin_unlock( &lock );
			entries[r] += 1;
		} // while
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	qsort( entries, RUNS, sizeof(size_t), compare );
	return (void *)median(entries);
} // Worker

void ctor() {
	lock = 0;
} // ctor

void dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=SpinLock Harness.c -lpthread -lm" //
// End: //
