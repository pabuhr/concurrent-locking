// Shared-memory mutual exclusion: major research trends since 1986, James H. Anderson, Yong-Jik Kim, Ted Herman
// Distrib. Comput. (2003) 16: 75-110, Fig 3.

#include<stdbool.h>

#define await( E ) while ( ! (E) ) Pause()

typedef struct {										// cache align array elements 
	VTYPE bit;
} Bit CALIGN;

typedef union {
	struct {
		Bit * last;
		VTYPE bit;
	};
	__int128 atom;
} Tailtype;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Bit * Slots;
static Tailtype Tail CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	Tailtype temp;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			temp.atom = __atomic_exchange_n( &Tail.atom, (Tailtype){ { &Slots[id], Slots[id].bit } }.atom, __ATOMIC_SEQ_CST );
			await( (*temp.last).bit != temp.bit );

			randomThreadChecksum += CriticalSection( id );

			Slots[id].bit = ! Slots[id].bit;

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		__sync_fetch_and_add( &sumOfThreadChecksums, randomThreadChecksum );

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
	Slots = Allocator( sizeof(typeof(Slots[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {			// initialize shared data
		Slots[i].bit = true;
	} // for
	Tail = (Tailtype){ { &Slots[0], (Bit){ false }.bit } };
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)Slots );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=GraunkeThakkarArray Harness.c -lpthread -latomic -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
