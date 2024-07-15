// Scalable version of Graunke and Thakkar without need for N static storage locations.
// John M. Mellor-Crummey and Michael L. Scott, Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors,
// ACM Transactions on Computer Systems, 9(1), February 1991, Fig. 4, 29.

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE * last CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define ptr( q ) ((VTYPE *)((TYPE)q & -2L))
#define bitset( p, v ) ((VTYPE *)((TYPE)p | v))
#define bitv( q ) ((TYPE)q & 1L)
#define deref( p ) (*p & 1)

_Atomic VTYPE start CALIGN = 0;

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	TYPE bit, mybit CALIGN;
	VTYPE * my = &mybit, * prev;

	if ( id == 0 ) {
		last = bitset( my, 1 );
		start = 1;
	} else { while ( ! start ); }						// other threads wait until last is initialized.

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			prev = Fas( last, (bitset( my, deref(my) )) );
			bit = bitv(prev);
			VTYPE * temp = ptr( prev );					// optimize
			await( (*temp & 1L) != bit );				// or eq.: await(deref(*temp) != bit); 

			randomThreadChecksum += CS( id );

			mybit ^= 1;									// or equivalently: * my = ! deref(my)

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		Fai( sumOfThreadChecksums, randomThreadChecksum );

		#ifdef FAST
		id = oid;
		#endif // FAST
		entries[r][id] = entry;
		Fai( Arrived, 1 );
		while ( stop != 0 ) Pause();
		Fai( Arrived, -1 );
	} // for

	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=GraunkeThakkarList2 Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
