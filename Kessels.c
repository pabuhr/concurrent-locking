// Joep L. W. Kessels, Arbitration Without Common Modifiable Variables, Acta Informatica, 17(2), 1982, pp. 140-141

typedef struct {
	TYPE Q[2], R[2];
} Token;
volatile Token *t;

#define inv( c ) (1 - c)
#define plus( a, b ) ((a + b) & 1)

static inline void binary_prologue( TYPE c, volatile Token *t ) {
	t->Q[c] = 1;
	Fence();											// force store before more loads
	t->R[c] = plus( t->R[inv(c)], c );
	Fence();											// force store before more loads
	while ( t->Q[inv( c )] && t->R[c] == plus( t->R[inv( c )], c ) ) Pause(); // busy wait
} // binary_prologue

static inline void binary_epilogue( TYPE c, volatile Token *t ) {
	t->Q[c] = 0;
} // binary_epilogue

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
	unsigned int n, e[N];
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
	// element 0 not used
	t = Allocator( sizeof(volatile Token) * N );
	for ( int i = 0; i < N; i += 1 ) {
		t[i].Q[0] = t[i].Q[1] = 0;
	} // for
} // ctor

void dtor() {
	free( (void *)t );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=Kessels Harness.c -lpthread -lm" //
// End: //
