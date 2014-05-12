volatile TYPE **x, **c;
int lN;

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	int high, j, l, rival;

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			j = 0;
			l = id;
			high = Clog2( N );							// maximal depth of binary tree
			while ( j < high ) {
				if ( l % 2 == 0 ) {
					x[j][l] = id;
					c[j][id] = 1;
					Fence();							// force store before more loads
					rival = x[j][l + 1];
					if ( rival != -1 ) {
						c[j][rival] = 0;
						Fence();						// force store before more loads
						while( c[j][id] != 0 ) Pause();
					}
				} else {
					x[j][l] = id;
					Fence();							// force store before more loads
				  yy:
					rival = x[j][l - 1];
					if ( rival != -1 ) {
						c[j][rival] = 0;
						Fence();						// force store before more loads
						while ( c[j][id] != 0 ) Pause();
						c[j][id] = 1;
						Fence();						// force store before more loads
						goto yy;
					} // if
				} // if
				l /= 2;
				j += 1;
			}
			CriticalSection( id );
			j = high;
			//int pow2 = pow( Degree, high );
			int pow2 = 1 << high;
			while ( j > 0 ) {
				j -= 1;
				pow2 /= Degree;
				l = id / pow2;
				x[j][l] = -1;
				int temp = (l % 2 == 0) ? l + 1 : l - 1;
				Fence();								// force store before more loads
				rival = x[j][temp];
				if ( rival != -1 ) {
					c[j][rival] = 0;
				}
			} // while
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
	Degree = 2;

	lN = N;
	if ( N % 2 == 1 ) lN += 1;

	x = Allocator( sizeof(volatile TYPE *) * lN );
	for ( int i = 0; i < lN; i += 1 ) {
		x[i] = Allocator( sizeof(TYPE) * lN );
	} // for
	c = Allocator( sizeof(volatile TYPE *) * lN );
	for ( int i = 0; i < lN; i += 1 ) {
		c[i] = Allocator( sizeof(TYPE) * lN );
	} // for
	for ( int i = 0; i < lN; i += 1 ) {					// initialize shared data
		for ( int j = 0; j < lN; j += 1 ) {
			x[i][j] = -1;
			c[i][j] = 0;
		} // for
	} // for
} // ctor

void dtor() {
	for ( int i = 0; i < N; i += 1 ) {
		free( (void *)c[i] );
	} // for
	for ( int i = 0; i < lN; i += 1 ) {
		free( (void *)x[i] );
	} // for
	free( (void *)c );
	free( (void *)x );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Zhang2T Harness.c -lpthread -lm" //
// End: //
