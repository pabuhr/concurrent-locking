static volatile TYPE Q[2];

#define await( E ) while ( ! (E) ) Pause()

enum { Z, F, T };

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
	TYPE temp;
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
			if ( id == 0 ) {
				temp = Q[1];
				Q[0] = temp == Z ? T : (temp == T ? T : F);
				Fence();
				temp = Q[1];
				Q[0] = temp == Z ? Q[0] : (temp == T ? T : F);
				Fence();
				await( Q[1] != Q[0] );
			} else {
				temp = Q[0];
				Q[1] = temp == Z ? T : (temp == T ? F : T);
				Fence();
				temp = Q[0];
				Q[1] = temp == Z ? Q[1] : (temp == T ? F : T);
				Fence();
				await( Q[0] == Z || Q[0] == Q[1] );
			} // for
			CriticalSection( id );
			Q[id] = Z;
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
	assert( N == 2 );
	Q[0] = Q[1] = Z;
} // ctor

void dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -std=gnu99 -O3 -DAlgorithm=Peterson2T Harness.c -lpthread -lm" //
// End: //
