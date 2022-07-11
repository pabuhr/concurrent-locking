// Dave Dice and Alex Kogan, TWA - Ticket Locks Augmented with a Waiting Array, Euro-Par 2019: Parallel Processing,
// 2019, Springer International Publishing, 334-345, Listing 1.1, page 338.
// https://doi.org/10.1007/978-3-030-29400-7_24
//
// The following implements the "TWA-ID" variant described in the appendix of https://arxiv.org/abs/1810.01573 (extended
// version)
//
// Derived from local "TKTWA7G" variant

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	volatile int32_t Ticket CALIGN;
	volatile int32_t Grant  CALIGN;
} TicketLock;

enum { NHOT = 1 };

static volatile intptr_t * ToSlot( int32_t TicketValue ) {
	// TODO: constexpr int NSLOTS = 4096
	// TODO: static_assert ((NSLOTS > 0 && (NSLOTS & (NSLOTS-1)) == 0);
	// TODO: size NSLOTS dynamically as a function of the number of CPUs
	enum { NSLOTS = 4096 };
	static volatile intptr_t WaitingArray[NSLOTS] __attribute__ ((aligned(8192))) ;

	// Hash the Lock identity "L" and the "TicketValue" into an array index
	// Note that this is a cache-conscious hash
	// In this simplfied implementation we don't include L.
	const int ix = (TicketValue * 127) & (NSLOTS-1);
	return WaitingArray + ix;
}

static inline void Acquire( TicketLock * L ) {
	const int32_t tx = Fai( &L->Ticket, 1 );
	int32_t dx = tx - L->Grant;

	// Fast-path : uncontended
	// dx reflects the number of threads waiting in front of our thread
  if ( LIKELY( dx == 0 ) ) return;

	// Slow-path : need to wait
	// NHOT is a tunable parameter and reflects the maximum number of threads
	// we want to allow to wait at any given time in the short-term classic
	// ticket lock waiting phase.
	// NHOT = +Inf conceptually degerates to classic ticket locks
	if ( dx > NHOT ) {
		// Long-term waiting
		int IdentityProxyAddress;
		const intptr_t ID = (intptr_t) &IdentityProxyAddress;
		volatile intptr_t * const restrict at = ToSlot( tx );
		// CONSIDER: instead of ST-FENCE just use SWAP/XCHG
		*at = ID;
		Fence();
		// ratify the previously observed state
		dx = tx - L->Grant;
		if (dx == 0) return;
		if (dx > NHOT) {
			await( UNLIKELY( *at != ID ) );
		}
		// Either true notification or false indication via hash collision We don't care which, just fall through into
		// the short-term mode For collisions we could be smarter and stay in the long-term phase.
	}

	// global short-term spinning
	// Usually at most one thread per lock in this state
	// Note that we could still use proportial back-off if desired
	// LFENCE makes particular sense for short-term waiting
	await( UNLIKELY( L->Grant == tx ) );
}

static inline void Release( TicketLock * L ) {
	// Don't actually need an atomic FAA here but we use one to avoid MESI S->M upgrades
	const int32_t k = Fai( &L->Grant, 1 ) + 1;

	// poke successor of successor, if any, to shift it from long-term to short-term waiting
	*ToSlot( k + NHOT ) = 0;
}

static TicketLock L[1] CALIGN;

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
	WO(Not yet properly fenced for weak memory models);

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			Acquire(L);
			WO( Fence(); );

			randomThreadChecksum += CS( id );

			Release(L);

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		Fai( &sumOfThreadChecksums, randomThreadChecksum );

		#ifdef FAST
		id = oid;
		#endif // FAST
		entries[r][id] = entry;
		Fai( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		Fai( &Arrived, -1 );
	} // for

	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	L->Ticket = 0;
	L->Grant  = 0;
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=TicketWaitingArray Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
