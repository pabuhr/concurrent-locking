// Gadi Taubenfeld, The Black-White Bakery Algorithm and Related Bounded-Space, Adaptive, Local-Spinning and FIFO Algorithms.
// Algorithm 2, page 62, DISC 2004, LNCS 3274, pp. 56-70, 2004

#include <stdbool.h>

typedef union {
	struct {
		volatile HALFSIZE color;
		volatile HALFSIZE number;
	};
	WHOLESIZE atom;										// ensure atomic assignment
	volatile WHOLESIZE vatom;							// volatile alias
} Ticket;
typedef enum { black, white } BW;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE color CALIGN;
static VTYPE * choosing CALIGN;
static Ticket * ticket CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define await( E ) while ( ! (E) ) Pause()

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	TYPE mycolor, number;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	typeof(&choosing[0]) mychoosing = &choosing[id];	// optimization
	typeof(&ticket[0]) myticket = &ticket[id];

	for ( int r = 0; r < RUNS; r += 1 ) {
		uint32_t randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			// step 1, select a ticket
			*mychoosing = true;							// entry protocol
			Fence();									// force store before more loads
			mycolor = color;
			number = 0;
			Ticket v;
			for ( typeof(N) j = 0; j < N; j += 1 ) {	// O(N) search for largest ticket
				v.atom = ticket[j].vatom;				// could change so must copy
				assert ( v.number <= N );
				if ( number < v.number && mycolor == v.color ) number = v.number;
			} // for
			number += 1;								// advance ticket
			myticket->atom = (Ticket){ {mycolor, number} }.atom; // set public state
			WO( Fence(); );
			*mychoosing = false;						// finished ticket selection
			Fence();									// force store before more loads

			// step 2, wait for ticket to be selected
			for ( typeof(N) j = 0; j < N; j += 1 ) {	// check other tickets
				typeof(&choosing[0]) otherchoosing = &choosing[j]; // optimization
				await( *otherchoosing == false );		// busy wait if thread selecting ticket
				WO( Fence(); );
				typeof(&ticket[0]) otherticket = &ticket[j]; // optimization
				if ( otherticket->color == mycolor ) {
					await( otherticket->number == 0 ||
						   otherticket->number > number || (otherticket->number >= number && j >= id) ||
						   otherticket->color != mycolor );
				} else {
					await( otherticket->number == 0 || mycolor != color ||
						   otherticket->color == mycolor );
				} // if
			} // for
			WO( Fence(); );

			randomThreadChecksum += CriticalSection( id );

			WO( Fence(); );
			color = ! mycolor;
			WO( Fence(); );
			myticket->number = 0;
			WO( Fence(); );

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			mychoosing = &choosing[id];					// optimization
			myticket = &ticket[id];
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
	choosing = Allocator( sizeof(typeof(choosing[0])) * N );
	ticket = Allocator( sizeof(typeof(ticket[0])) * N );
	for ( typeof(N) i = 0; i < N; i += 1 ) {			// initialize shared data
		choosing[i] = false;
		ticket[i] = (Ticket){ {black, 0} };
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)ticket );
	free( (void *)choosing );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=TaubenfeldBWBakery Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
