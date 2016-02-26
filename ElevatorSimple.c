#include <stdbool.h>

static volatile TYPE fast CALIGN, first CALIGN;
static volatile TYPE *apply CALIGN;
static volatile TYPE *b CALIGN, x CALIGN, y CALIGN;
//static volatile TYPE curr CALIGN;

static TYPE PAD CALIGN __attribute__(( unused ));		// protect further false sharing


#define await( E ) while ( ! (E) ) Pause()

#ifdef CAS

#define WCas( x ) __sync_bool_compare_and_swap( &(fast), false, true )

#else // ! CAS

#if defined( WCas1 )

static inline bool WCas( TYPE id ) {
	b[id] = true;
	x = id;
	Fence();											// force store before more loads
	if ( FASTPATH( y != N ) ) {
		b[id] = false;
		return false;
	} // if
	y = id;
	Fence();											// force store before more loads
	if ( FASTPATH( x != id ) ) {
		b[id] = false;
		Fence();										// force store before more loads
		for ( int j = 0; j < N; j += 1 )
			await( ! b[j] );
		if ( FASTPATH( y != id ) ) return false;
	} // if
	bool leader = ((! fast) ? fast = true : false);
	y = N;
	b[id] = false;
	return leader;
} // WCas

#elif defined( WCas2 )

static inline bool WCas( TYPE id ) {
	b[id] = true;
	Fence();											// force store before more loads
	for ( typeof(id) thr = 0; thr < id; thr += 1 ) {
		if ( FASTPATH( b[thr] ) ) {
			b[id] = false;
			return false ;
		} // if
	} // for
	for ( typeof(id) thr = id + 1; thr < N; thr += 1 ) {
		await( ! b[thr] );
	} // for
	bool leader = ((! fast) ? fast = true : false);
	b[id] = false;
	return leader;
} // WCas

#else
    #error unsupported architecture
#endif // WCas

#endif // CAS


static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST
	typeof(id) thr;

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			apply[id] = true;
			if ( FASTPATH( WCas( id ) ) ) {
				Fence();								// force store before more loads
				await( first == N || first == id );
				first = id;
				fast = false;
			} else {
				await( first == id );
			} // if

			CriticalSection( id );

			for ( thr = cycleUp( id, N ); ! apply[thr]; thr = cycleUp( thr, N ) );
//			for ( thr = cycleUp( curr, N ); ! apply[thr]; thr = cycleUp( thr, N ) );
//			curr = thr;
			apply[id] = false;							// must appear before setting first
			first = (thr == id) ? N : thr;
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

void __attribute__((noinline)) ctor() {
	b = Allocator( N * sizeof(typeof(b[0])) );
	apply = Allocator( N * sizeof(typeof(apply[0])) );
	for ( TYPE id = 0; id < N; id += 1 ) {				// initialize shared data
		apply[id] = b[id] = false;
	} // for
	y = first = N;
//	curr = N;
	fast = false;
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)apply );
	free( (void *)b );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=ElevatorSimple -DWCas1 Harness.c -lpthread -lm" //
// End: //
