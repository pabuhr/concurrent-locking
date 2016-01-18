pthread_mutex_t lock CALIGN;

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			pthread_mutex_lock( &lock );
			CriticalSection( id );
			pthread_mutex_unlock( &lock );
			entry += 1;
		} // while
		entries[r][id] = entry;
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	return NULL;
} // Worker

void ctor() {
	pthread_mutex_init( &lock, NULL );
} // ctor

void dtor() {
	pthread_mutex_destroy( &lock );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=PthreadLock Harness.c -lpthread -lm" //
// End: //
