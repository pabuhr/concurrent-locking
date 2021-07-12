static VTYPE Q[2];

#define await( E ) while ( ! (E) ) Pause()

enum { Z, F, T };

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	TYPE temp;

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
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
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			entry += 1;
		} // while
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

void ctor() {
	assert( N == 2 );
	Q[0] = Q[1] = Z;
} // ctor

void dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Peterson2T Harness.c -lpthread -lm" //
// End: //
