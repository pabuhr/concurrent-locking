#include <stdbool.h>

static volatile TYPE first CALIGN;
static volatile TYPE *apply CALIGN;
static volatile TYPE *b CALIGN;

static TYPE PAD CALIGN __attribute__(( unused ));		// protect further false sharing


#define await( E ) while ( ! (E) ) Pause()

static inline void mutexB( TYPE id ) {
	b[id] = true;
	Fence();											// force store before more loads
	for ( int kk = 0; kk < id; kk += 1 ) {
		if ( b[kk] || first == id ) {
			b[id] = false;
			await( first == id );
			return;
		} // if
	} // for
	for ( int kk = id + 1; kk < N; kk += 1 ) {
		await( ! b[kk] || first == id );
	  if ( first == id ) break;
	} // for
	await( first == id || first == N );
	first = id;
	b[id] = false;
} // mutexB


static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			apply[id] = true;
			mutexB( id );
			apply[id] = false;

			CriticalSection( id );

			typeof(id) kk = cycleUp( id, N );
			while ( kk != id && ! apply[kk] )
				kk = cycleUp( kk, N );
			first = kk == id ? N : kk;
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
	first = N;
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)apply );
	free( (void *)b );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=ElevatorBurns Harness.c -lpthread -lm" //
// End: //
