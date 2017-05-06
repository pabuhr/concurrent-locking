#include <stdbool.h>

typedef struct CALIGN {									// performance gain when fields juxtaposed
	TYPE apply;
#ifdef FLAG
	TYPE flag;
#endif // FLAG
} Tstate;

static volatile TYPE fast CALIGN;
static volatile Tstate *tstate CALIGN;

#ifndef CAS
static volatile TYPE *b CALIGN, x CALIGN, y CALIGN;
#endif // ! CAS

#ifndef FLAG
static volatile TYPE first CALIGN;
#endif // FLAG

static TYPE PAD CALIGN __attribute__(( unused ));		// protect further false sharing


#define await( E ) while ( ! (E) ) Pause()

#ifdef CAS

#define WCas( x ) __sync_bool_compare_and_swap( &(fast), false, true )

#elif defined( WCasBL )

static inline bool WCas( TYPE id ) {					// based on Burns-Lamport algorithm
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

#elif defined( WCasLF )

static inline bool WCas( TYPE id ) {					// based on Lamport-Fast algorithm
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
		for ( int k = 0; k < N; k += 1 )
			await( ! b[k] );
		if ( FASTPATH( y != id ) ) return false;
	} // if
	bool leader = ((! fast) ? fast = true : false);
	y = N;
	b[id] = false;
	return leader;
} // WCas

#else
    #error unsupported architecture
#endif // WCas


static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	volatile typeof(tstate[0].apply) *applyId = &tstate[id].apply;
#ifdef FLAG
	volatile typeof(tstate[0].flag) *flagId = &tstate[id].flag;
	volatile typeof(tstate[0].flag) *flagN = &tstate[N].flag;
#endif // FLAG

#ifdef FAST
	typeof(id) cnt = 0, oid = id;
#endif // FAST
	typeof(id) thr;

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			*applyId = true;							// entry protocol
			if ( FASTPATH( WCas( id ) ) ) {				// true => leader
#ifndef CAS
				Fence();								// force store before more loads
#endif // ! CAS

#ifdef FLAG
				await( *flagId || *flagN );
				*flagN = false;
#else
				await( first == id || first == N );
				first = id;
#endif // FLAG
				fast = false;
			} else {
#ifdef FLAG
				await( *flagId );
#else
				await( first == id );
#endif // FLAG
			} // if
#ifdef FLAG
			*flagId = false;
#endif // FLAG

			CriticalSection( id );

#ifdef CYCLEUP
			for ( thr = cycleUp( id, N ); ! tstate[thr].apply; thr = cycleUp( thr, N ) );
#else // CYCLEDOWN
			#define Mod( a, b, N ) ((a + b) < N ? (a + b) : (a + b - N))
			for ( thr = N - 1; ! tstate[Mod(id, thr, N)].apply; thr -= 1 );
#endif // CYCLEUP

			*applyId = false;							// must appear before setting first

#ifdef CYCLEUP
			if ( FASTPATH( thr != id ) )
#ifdef FLAG
				tstate[thr].flag = true; else *flagN = true;
#else // NOFLAG
				first = thr; else first = N;
#endif // FLAG

#else // CYCLEDOWN

			if ( FASTPATH( thr != 0 ) )
#ifdef FLAG
				tstate[Mod(id, thr, N)].flag = true; else *flagN = true;
#else // NOFLAG
				first = Mod(id, thr, N); else first = N;
#endif // FLAG
#endif // CYCLEUP

#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );

			applyId = &tstate[id].apply;				// must reset for new id
#ifdef FLAG
			flagId = &tstate[id].flag;
			flagN = &tstate[N].flag;
#endif // FLAG
#endif // FAST
			entry += 1;
		} // while
#ifdef FAST
		id = oid;

		applyId = &tstate[id].apply;					// must reset for new id
#ifdef FLAG
		flagId = &tstate[id].flag;
		flagN = &tstate[N].flag;
#endif // FLAG
#endif // FAST
		entries[r][id] = entry;
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	tstate = Allocator( (N + 1) * sizeof(typeof(tstate[0])) );
	for ( TYPE id = 0; id <= N; id += 1 ) {				// initialize shared data
		tstate[id].apply = false;
#ifdef FLAG
		tstate[id].flag = false;
#endif // FLAG
	} // for

#ifdef FLAG
	tstate[N].flag = true;
#else
	first = N;
#endif // FLAG

#ifndef CAS
	b = Allocator( N * sizeof(typeof(b[0])) );
	for ( TYPE id = 0; id < N; id += 1 ) {				// initialize shared data
		b[id] = false;
	} // for
	y = N;
#endif // CAS
	fast = false;
} // ctor

void __attribute__((noinline)) dtor() {
#ifndef CAS
	free( (void *)b );
#endif // ! CAS
	free( (void *)tstate );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=ElevatorLinear -DWCasLF Harness.c -lpthread -lm" //
// End: //
