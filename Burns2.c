// James E. Burns, Symmetry in Systems of Asynchronous Processes. 22nd Annual
// Symposium on Foundations of Computer Science, 1981, Figure 2, p 170.

#include <stdbool.h>

static volatile TYPE turn CALIGN, *flag CALIGN;
static TYPE PAD CALIGN __attribute__(( unused ));		// protect further false sharing

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	int j;

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
#if defined( __sparc )
			__asm__ __volatile__ ( "" : : : "memory" );
#endif // __sparc
		  L0: flag[id] = true;							// entry protocol
			turn = id;									// RACE
			Fence();									// force store before more loads
		  L1: if ( FASTPATH( turn != id ) ) {
				flag[id] = false;
				Fence();								// force store before more loads
			  L11: for ( j = 0; j < N; j += 1 )
					if ( j != id && flag[j] ) { Pause(); goto L11; }
				goto L0;
			} else {
//				flag[id] = true;
//				Fence();								// force store before more loads
			  L2: if ( FASTPATH( turn != id ) ) goto L1;
				for ( j = 0; j < N; j += 1 )
					if ( FASTPATH( j != id && flag[j] ) ) goto L2;
			} // if
			CriticalSection( id );
			flag[id] = false;							// exit protocol
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
	flag = Allocator( N * sizeof(typeof(flag[0])) );
	for ( int i = 0; i < N; i += 1 ) {					// initialize shared data
		flag[i] = false;
	} // for
	//turn = 0;
} // ctor

void dtor() {
	free( (void *)flag );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Burns2 Harness.c -lpthread -lm" //
// End: //
