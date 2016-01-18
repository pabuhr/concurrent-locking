// Xiaodong Zhang, Yong Yan and Robert Castaneda, Evaluating and Designing Software Mutual Exclusion Algorithms on
// Shared-Memory Multiprocessors, Parallel Distributed Technology: Systems Applications, IEEE, 1996, 4(1), Figure 5,
// p. 31

static volatile TYPE **c CALIGN, **p CALIGN, **t CALIGN;

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	int rival, j, ridi, ridt, high;

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		high = Clog2( N );								// maximal depth of binary tree
		//printf( "id:%d high:%d\n", id, high );
		while ( stop == 0 ) {
			for ( j = 0; j < high; j += 1 ) {
				ridi = id >> j;							// round id for intent
				ridt = ridi >> 1;						// round id for turn
				c[j][ridi] = id;
				t[j][ridt] = id;
				p[j][id] = 0;
				Fence();
				rival = c[j][ridi ^ 1];
				//printf( "1 id:%d j:%d, rival:%d\n", id, j, rival );
				if ( rival != -1 ) {
					if ( t[j][ridt] == id ) {
						if ( p[j][rival] == 0 ) {
							p[j][rival] = 1;
							Fence();
						} // if
						while ( p[j][id] == 0 ) Pause();
						if ( t[j][ridt] == id ) {
							while ( p[j][id] <= 1 ) Pause();
						}
					} // if
				} // if
			} // for
			CriticalSection( id );
			for ( j = high - 1; j >= 0; j -= 1 ) {
				c[j][id / (1 << j)] = -1;
				Fence();
				rival = t[j][id / (1 << (j + 1))];
				//printf( "2 id:%d j:%d, rival:%d %ld %ld\n", id, j, rival, t[j][0], t[j][1] );
				if ( rival != id ) {
					p[j][rival] = 2;
				}
			} // while
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
	c = Allocator( sizeof(typeof(c[0])) * N );
	p = Allocator( sizeof(typeof(p[0])) * N );
	t = Allocator( sizeof(typeof(t[0])) * N );
	for ( int i = 0; i < N; i += 1 ) {
		c[i] = Allocator( sizeof(typeof(c[0][0])) * (N+1) );
		p[i] = Allocator( sizeof(typeof(p[0][0])) * (N+1) );
		t[i] = Allocator( sizeof(typeof(t[0][0])) * (N+1) );
		for ( int j = 0; j < (N+1); j += 1 ) {
			c[i][j] = -1;
			p[i][j] = 0;
		} // for
//		c[i] = v[i] = x[i] = 0;
	} // for
} // ctor

void dtor() {
	for ( int i = 0; i < N; i += 1 ) {
		free( (void *)t[i] );
		free( (void *)p[i] );
		free( (void *)c[i] );
	} // for
	free( (void *)t );
	free( (void *)p );
	free( (void *)c );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=ZhangYA Harness.c -lpthread -lm" //
// End: //
