pthread_mutex_t lock;

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
			pthread_mutex_lock( &lock );
			CriticalSection( id );
			pthread_mutex_unlock( &lock );
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
	pthread_mutex_init( &lock, NULL );
} // ctor

void dtor() {
	pthread_mutex_destroy( &lock );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=PthreadLock Harness.c -lpthread -lm" //
// End: //
