// Joep L. W. Kessels, Arbitration Without Common Modifiable Variables, Acta Informatica, 17(2), 1982, pp. 140-141

typedef struct CALIGN {
	TYPE Q[2],
#if defined( PETERSON )
		R;
#else // default Kessels' read race
		R[2];
#endif // PETERSON
} Token;

static volatile Token *t CALIGN;
static TYPE PAD CALIGN __attribute__(( unused ));		// protect further false sharing

#define inv( c ) ( (c) ^ 1 )
#define plus( a, b ) ((a + b) & 1)

static inline void binary_prologue( TYPE c, volatile Token *t ) {
	TYPE other = inv( c );
#if defined( PETERSON )
	t->Q[c] = 1;
	t->R = c;
	Fence();											// force store before more loads
	while ( t->Q[other] && t->R == c ) Pause();			// busy wait
#else // default Kessels' read race
	t->Q[c] = 1;
	Fence();											// force store before more loads
	t->R[c] = plus( t->R[other], c );
	Fence();											// force store before more loads
	while ( t->Q[other] && t->R[c] == plus( t->R[other], c ) ) Pause(); // busy wait
#endif // PETERSON
} // binary_prologue

static inline void binary_epilogue( TYPE c, volatile Token *t ) {
	t->Q[c] = 0;
} // binary_epilogue

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	unsigned int n, e[N];

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			n = N + id;
			while ( n > 1 ) {							// entry protocol
#if 1
//				int lr = n % 2;
				int lr = n & 1;
//				n = n / 2;
				n >>= 1;
				binary_prologue( lr, &t[n] );
				e[n] = lr;
#else
				binary_prologue( n % 2, &t[n / 2] );
				e[n / 2] = n % 2;
				n = n / 2;
#endif
			} // while
			CriticalSection( id );

			for ( n = 1; n < N; n = n + n + e[n] ) {	// exit protocol
//				n = n + n + e[n];
//				binary_epilogue( n & 1, &t[n >> 1] );
				binary_epilogue( e[n], &t[n] );
			} // for
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
	// element 0 not used
	t = Allocator( sizeof(typeof(t[0])) * N );
	for ( int i = 0; i < N; i += 1 ) {
		t[i].Q[0] = t[i].Q[1] = 0;
	} // for
} // ctor

void dtor() {
	free( (void *)t );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Kessels Harness.c -lpthread -lm" //
// End: //
