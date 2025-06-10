// Fissile locks
// The outer lock is a classic ticket lock
// The inner lock is 2X lanes of Reciprocating lock 
// 
// Fissile Locks :
// Dave Dice and Alex Kogan
// NETYS 2020 
// https://doi.org/10.1007/978-3-030-67087-0_13
// https://arxiv.org/abs/2003.05025 (long form) 
//
// Reciprocating Locks : 
// Dave Dice and Alex Kogan
// Reciprocating Locks
// PPoPP 2025
// https://doi.org/10.1145/3710848.3710862 (ACM)
// The code below closely follows Listing-1 in the above.
// https://arxiv.org/abs/2501.02380 (Long form)
// The Reciprocating lock gains performance by a controlled (small) amount of unfairness.

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdatomic.h>

typedef struct _WaitElement {
	_Atomic(struct _WaitElement *) Gate CALIGN;
} WaitElement;

static WaitElement * const LOCKEDEMPTY = (WaitElement *)(uintptr_t) 1;

typedef struct {
	_Atomic(WaitElement *) Arrivals;
} ReciprocatingLock;

static __thread WaitElement E CALIGN;

// Note that we can extract more performance by sequestering
// the constituent elements onto dedicated cache lines(sector) via CALIGN,
// in order to reduce false sharing.  
// This inflates the size of the lock, so in this particular implementation
// we sacrifice some throughput for size. 
// We align the composite lock instance, however.  

typedef struct {
	_Atomic(uint64_t) Ticket;
	_Atomic(uint64_t) Grant; 
	ReciprocatingLock Lanes [2]; 
} FissileLock; 

static FissileLock lock CALIGN;

static inline WaitElement * Acquire( ReciprocatingLock * lock, WaitElement ** _EndOfSegment ) {
	Str( E.Gate, NULL );
	WaitElement * succ = NULL;
	WaitElement * EndOfSegment = &E;
	WaitElement * const tail = Fas( lock->Arrivals, &E );
	assert( tail != &E );
	if ( tail != NULL ) {
		// coerce LOCKEDEMPTY to null
		// succ will be our successor when we subsequently release
		succ = (WaitElement *)(((uintptr_t) tail) & ~1);
		assert(succ != &E);

		// contention : waiting phase
		// Consider : could use HemLock CTR optimization here and spin using exchange
		// That, in turn, would obviate the need to clear Gate at the top of Acquire
		// and would avoid the MESI/MOESI/MESIF S->M coherence upgrade.
		for ( ;; ) {
			EndOfSegment = Ld( E.Gate );
			if ( EndOfSegment != NULL ) break;
			Pause();
		}
		assert( EndOfSegment != &E );

		// Detect logical end-of-segment terminus address
		if ( succ == EndOfSegment ) {
			succ = NULL;								// quash
			EndOfSegment = LOCKEDEMPTY;
		}
	}

	*_EndOfSegment = EndOfSegment;
	return succ;
}

static inline void Release( ReciprocatingLock * lock, WaitElement * EndOfSegment, WaitElement * succ ) {
	assert( EndOfSegment != NULL );
	assert( Ld( lock->Arrivals ) != NULL );

	if ( succ != NULL ) {
		assert( Ld( succ->Gate ) == NULL );
		Str( succ->Gate, EndOfSegment );
		return;
	}

	assert( EndOfSegment == LOCKEDEMPTY || EndOfSegment == &E );
#if 0
	WaitElement * v = EndOfSegment;
	if ( Casv( lock->Arrivals, v, NULL ) ) {
		// uncontended fast-path return
		return;
	}
#else
	if ( Ld( lock->Arrivals ) == EndOfSegment ) {
		WaitElement * v = EndOfSegment;
		if ( Casv( lock->Arrivals, v, NULL ) ) {
			// uncontended fast-path return
			return;
		}
	}
#endif

	WaitElement * w = Fas( lock->Arrivals, LOCKEDEMPTY );
	assert( w != NULL );
	assert( w != LOCKEDEMPTY );
	assert( w != &E );
	assert( Ld( w->Gate ) == NULL );
	Str( w->Gate, EndOfSegment );
}

// We feed the ticket value into Mix32A(), yielding what is effectively
// a counter-based PRNG (CBRN).
// ultimately, we just need a "decent" hash function but are willing
// to trade quality for latency.
// Mix32A, PCG32, Phi32, or Marsaglia XOR-Shift are all viable candidates.  
// Mix32A doesn't have much internal available ILP, so is perhaps not the best choice.  

static uint32_t Mix32A (uint32_t v) {
	static const int32_t Mix32KA = 0x9abe94e3;
	v = (v ^ (v >> 16)) * Mix32KA; 
	v = (v ^ (v >> 16)) * Mix32KA; 
	return v ^ (v >> 16);  
} 

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

    // Note that E could safely either reside in TLS or, for this particular use case, on-stack 

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

            ReciprocatingLock * Lane = NULL; 
            WaitElement * succ = NULL; 
            WaitElement * EndOfSegment = NULL; 

            // Try opportunistic barging on outer ticket lock 
            // With 32-bit ticket variables our "trylock" on the ticket lot would be vulnerable 
            // to wrap-around aliasing, so we require 64-bit variables
            // where wrap-around is not a practical concern.  
            uint64_t G = Ld( lock.Grant );
            uint64_t tx = G; 
		    if ( ! Casv( lock.Ticket, tx, G+1 ) ) {
				// opportunistic optimistic fast-path bypass failed
				// Revert to slow path -- contention -- waiting
				// Select one of the lanes and acquire the associated reciprocating lock instance.
				// Using 2 lanes and randomization promotes long-term statistical 
				// admission fairness. 
				// We intentionally favor Lane[0] as this in turn leaves the ticket lock window
				// open for longer periods than would balanced (or uniform random) dispersal, 
				// which in turn enables the fast path to be used more often by arrivals.  
				// The downside is that the fast path success rate and fairness is topology- 
				// and platform-specific, so we have a trade-off.  
				// Almost all residual long-term unfairness arises by virtue of the fast path
				// and _not from the reciprocating palindromic admission schedule.  
				// The probability P < 10/256 (a tunable parameter) in the Bernoulli trial below 
				// influences both long-term statistical fairness and "saturation" of the ticket lock
				// and the availability of the fast path.  
				int ix = (Mix32A(G) & 0xFF) < 10; 
				Lane = lock.Lanes + ix;
				succ = Acquire (Lane, &EndOfSegment);

				// Next, acquire the outer ticket lock 
				// At most 2 threads can compete for the outer ticket lock at any given time
				// So we have global spinning, but the population is bounded.  
				// Hand-over is extremely fast.  
				tx = Fai( lock.Ticket, 1 ); 
				while ( Ld( lock.Grant ) != tx) {
					Pause();
				}
            }  

			randomThreadChecksum += CS( id );

            // Release the outer ticket lock
            assert (Ld( lock.Grant ) == tx); 
            Str( lock.Grant, tx + 1 ); 

            // If necessary, release the selected inner lane lock 
            if ( Lane != NULL ) { 
				Release( Lane, EndOfSegment, succ );
            }

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
    Str( lock.Ticket, 0 ); 
    Str( lock.Grant , 0 ); 
	Str( lock.Lanes[0].Arrivals, NULL );
	Str( lock.Lanes[1].Arrivals, NULL );
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=FissileTicket Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
