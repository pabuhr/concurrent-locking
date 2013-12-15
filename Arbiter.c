static volatile TYPE *intents, *serving;				// shared
static volatile TYPE arbiter_stop = 0;
static pthread_t arbiter;

// Worker id is only allowed into CS by the assignment serving[id] = 1 by the arbiter.  After CS, id sets serving[id] =
// 0.  It cannot reclaim CS until the arbiter has resumed its activity by exiting from while, and entering for.  There
// it advances to the next id, and cycles through the other workers, before it can reach id again.

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
			intents[id] = 1;							// entry protocol
			while ( serving[id] == 0 ) Pause();			// busy wait
			CriticalSection( id );
			serving[id] = 0;							// exit protocol
			entries[r] += 1;
		} // while
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	qsort( entries, RUNS, sizeof(size_t), compare );
	return (void *)median(entries);
} // Worker

void *Arbiter( void *arg ) {
	int id = N;											// force cycle to start at id=0
	for ( ;; ) {
		for ( ;; ) {									// circular search => no starvation
			id = cycleUp( id, N );
		  if ( intents[id] == 1 ) break;				// want in ?
	  if ( arbiter_stop == 1 ) goto Fini;
			Pause();
		} // for
		intents[id] = 0;								// retract intent on behalf of worker
		serving[id] = 1;								// wait for exit from critical section
		while ( serving[id] == 1 ) Pause();				// busy wait
	} // for
  Fini: ;
	return 0;
} // Arbiter

void ctor() {
	intents = Allocator( sizeof(volatile TYPE) * N );
	serving = Allocator( sizeof(volatile TYPE) * N );
	for ( int i = 0; i < N; i += 1 ) {					// initialize shared data
		intents[i] = serving[i] = 0;
	} // for

	if ( pthread_create( &arbiter, NULL, Arbiter, NULL ) != 0 ) abort();
} // ctor

void dtor() {
	arbiter_stop = 1;
	if ( pthread_join( arbiter, NULL ) != 0 ) abort();

	free( (void *)serving );
	free( (void *)intents );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=Arbiter Harness.c -lpthread -lm" //
// End: //
